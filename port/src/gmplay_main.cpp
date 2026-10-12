// Entry point of the port. The original main() is compiled as gm_game_main (see shim/gmcompat.h) and
// reached through gm_main_entry in PLAYGAME.C, because its parameters are 16-bit ints there.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

extern "C" int32_t gm_main_entry(int32_t argc, char **argv);
extern "C" volatile uint32_t gm_heartbeat;      // bumped by gm_pump(); the watchdog below watches it

#ifdef _WIN32
// Diagnostics: a crash reporter and a hang watchdog that print symbolised call stacks
// (they need the PDB next to the exe).
#define NOMINMAX
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

static void print_stack(HANDLE thread, CONTEXT ctx)
{
    HANDLE proc = GetCurrentProcess();
    STACKFRAME64 sf = {};
    sf.AddrPC.Offset = ctx.Rip;    sf.AddrPC.Mode = AddrModeFlat;
    sf.AddrFrame.Offset = ctx.Rbp; sf.AddrFrame.Mode = AddrModeFlat;
    sf.AddrStack.Offset = ctx.Rsp; sf.AddrStack.Mode = AddrModeFlat;
    for (int i = 0; i < 24 && StackWalk64(IMAGE_FILE_MACHINE_AMD64, proc, thread, &sf, &ctx, NULL,
                                           SymFunctionTableAccess64, SymGetModuleBase64, NULL); i++) {
        char buf[sizeof(SYMBOL_INFO) + 256];
        SYMBOL_INFO *sym = (SYMBOL_INFO *)buf;
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen = 255;
        DWORD64 disp = 0;
        IMAGEHLP_LINE64 line = { sizeof(line) };
        DWORD ldisp = 0;
        const char *name = SymFromAddr(proc, sf.AddrPC.Offset, &disp, sym) ? sym->Name : "?";
        if (SymGetLineFromAddr64(proc, sf.AddrPC.Offset, &ldisp, &line))
            fprintf(stderr, "  #%d %s  (%s:%lu)\n", i, name, line.FileName, line.LineNumber);
        else
            fprintf(stderr, "  #%d %s\n", i, name);
    }
    fflush(stderr);
}

static void init_symbols()
{
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    SymInitialize(GetCurrentProcess(), NULL, TRUE);
}

static LONG WINAPI crash_filter(EXCEPTION_POINTERS *ep)
{
    init_symbols();
    fprintf(stderr, "\n*** crash: exception %08lX at %p", ep->ExceptionRecord->ExceptionCode, ep->ExceptionRecord->ExceptionAddress);
    if (ep->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && ep->ExceptionRecord->NumberParameters >= 2)
        fprintf(stderr, " (%s address %p)", ep->ExceptionRecord->ExceptionInformation[0] ? "write to" : "read of",
                (void *)ep->ExceptionRecord->ExceptionInformation[1]);
    fprintf(stderr, "\n");
    print_stack(GetCurrentThread(), *ep->ContextRecord);
    return EXCEPTION_EXECUTE_HANDLER;
}

// The C runtime calls this for invalid arguments (fopen(NULL), a closed FILE*, ...) instead of failing quietly.
static void invalid_parameter(const wchar_t *, const wchar_t *, const wchar_t *, unsigned, uintptr_t)
{
    init_symbols();
    fprintf(stderr, "\n*** invalid parameter passed to a C runtime function; call stack:\n");
    CONTEXT ctx = {};
    RtlCaptureContext(&ctx);
    print_stack(GetCurrentThread(), ctx);
    ExitProcess(4);
}

// GM_EXITTRACE=1: print who called exit() (the stack is still intact inside an atexit handler).
static void trace_exit()
{
    init_symbols();
    fprintf(stderr, "\n*** exit() called from:\n");
    CONTEXT ctx = {};
    RtlCaptureContext(&ctx);
    print_stack(GetCurrentThread(), ctx);
}

static HANDLE main_thread;

static DWORD WINAPI watchdog(LPVOID)
{
    uint32_t last = gm_heartbeat;
    int stalled = 0;
    for (;;) {
        Sleep(1000);
        if (gm_heartbeat != last) { last = gm_heartbeat; stalled = 0; continue; }
        if (++stalled < 5) continue;
        SuspendThread(main_thread);
        CONTEXT ctx = {};
        ctx.ContextFlags = CONTEXT_FULL;
        GetThreadContext(main_thread, &ctx);
        init_symbols();
        fprintf(stderr, "\n*** hang: the engine has not reached the platform layer for %d s; main thread is in:\n", stalled);
        print_stack(main_thread, ctx);
        ExitProcess(3);
    }
}
#elif !defined(__EMSCRIPTEN__)
// POSIX diagnostics, same idea as above: a crash reporter, a hang watchdog and an exit tracer, printing call stacks
// with backtrace() (function names need the executable to export its symbols; CMake adds -rdynamic on Linux).
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#if defined(__has_include)
#  if __has_include(<execinfo.h>)
#    include <execinfo.h>
#    define GM_HAVE_BACKTRACE 1
#  endif
#endif

static void print_stack()
{
#ifdef GM_HAVE_BACKTRACE
    void *frames[32];
    int n = backtrace(frames, 32);
    backtrace_symbols_fd(frames, n, 2);
#else
    static const char msg[] = "  (no backtrace() on this platform)\n";
    if (write(2, msg, sizeof msg - 1) < 0) {}
#endif
}

static void crash_handler(int sig)
{
    char msg[64];
    int n = snprintf(msg, sizeof msg, "\n*** crash: signal %d\n", sig);
    if (write(2, msg, (size_t)n) < 0) {}
    print_stack();
    _exit(128 + sig);
}

static void trace_exit()
{
    static const char msg[] = "\n*** exit() called from:\n";
    if (write(2, msg, sizeof msg - 1) < 0) {}
    print_stack();
}

static pthread_t main_thread;

static void hang_handler(int)
{
    static const char msg[] = "\n*** hang: the engine has not reached the platform layer for 5 s; main thread is in:\n";
    if (write(2, msg, sizeof msg - 1) < 0) {}
    print_stack();
    _exit(3);
}

static void *watchdog(void *)
{
    uint32_t last = gm_heartbeat;
    int stalled = 0;
    for (;;) {
        sleep(1);
        if (gm_heartbeat != last) { last = gm_heartbeat; stalled = 0; continue; }
        if (++stalled >= 5) pthread_kill(main_thread, SIGUSR2);
    }
    return NULL;
}
#endif

// ---------------------------------------------------------------------------------------------------------------
// Launcher: "gmplay [options] [game]". A game is a .gam file, a folder holding one, or a name looked up in the game
// folders (GM_GAMES, then the repository's cd/gameware next to the build, then the current folder). The engine reads and
// writes the game's files (scores, saved games, recordings) relative to the current folder, as it did in DOS, so the
// launcher changes into the game's folder and hands the engine just the .gam name.
// ---------------------------------------------------------------------------------------------------------------
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>
#include "settings.h"
#include "audio.h"

namespace fs = std::filesystem;

static std::string lower(std::string s)
{
    for (char &c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

// Case-insensitive lookup of one name inside a folder (DOS-era games ship upper-case names).
static fs::path find_ci(const fs::path &dir, const std::string &name)
{
    std::error_code ec;
    if (fs::exists(dir / name, ec)) return dir / name;
    for (const auto &e : fs::directory_iterator(dir, ec))
        if (lower(e.path().filename().string()) == lower(name)) return e.path();
    return {};
}

static bool has_ext_ci(const fs::path &p, const char *ext) { return lower(p.extension().string()) == ext; }

// the .gam file in a folder: the one named like the folder if there are several
static fs::path gam_in_folder(const fs::path &dir)
{
    std::error_code ec;
    std::vector<fs::path> all;
    for (const auto &e : fs::directory_iterator(dir, ec))
        if (e.is_regular_file(ec) && has_ext_ci(e.path(), ".gam")) all.push_back(e.path());
    if (all.empty()) return {};
    std::sort(all.begin(), all.end());
    std::string folder = lower(fs::absolute(dir).filename().string());
    for (const auto &p : all) if (lower(p.stem().string()) == folder) return p;
    return all.front();
}

static fs::path exe_dir(const char *argv0)
{
    std::error_code ec;
#ifdef _WIN32
    char buf[MAX_PATH];
    DWORD n = GetModuleFileNameA(NULL, buf, MAX_PATH);
    if (n > 0 && n < MAX_PATH) return fs::path(buf).parent_path();
#elif defined(__linux__)
    fs::path p = fs::read_symlink("/proc/self/exe", ec);
    if (!ec) return p.parent_path();
#elif defined(__APPLE__)
    char buf[4096];
    uint32_t size = sizeof buf;
    if (_NSGetExecutablePath(buf, &size) == 0) return fs::weakly_canonical(fs::path(buf), ec).parent_path();
#endif
    return fs::absolute(fs::path(argv0), ec).parent_path();
}

static std::vector<fs::path> game_roots(const char *argv0)
{
    std::vector<fs::path> roots;
    if (const char *env = getenv("GM_GAMES")) {                  // a list separated by ';' (or ':' off Windows)
        std::string s = env, cur;
        for (char c : s + ";") {
            if (c == ';' || (c == ':' && s.size() > 2 && cur.size() > 1)) { if (!cur.empty()) roots.push_back(cur); cur.clear(); }
            else cur += c;
        }
    }
    fs::path exe = exe_dir(argv0);
    for (const char *rel : { "../../cd/gameware", "../cd/gameware", "cd/gameware", "games" }) roots.push_back(exe / rel);
    roots.push_back(fs::current_path());
    std::error_code ec;
    roots.erase(std::remove_if(roots.begin(), roots.end(), [&](const fs::path &p) { return !fs::is_directory(p, ec); }), roots.end());
    return roots;
}

// Finds the .gam for `arg`; empty if there is none.
static fs::path resolve_game(const char *argv0, const std::string &arg)
{
    std::error_code ec;
    fs::path p(arg);
    if (fs::is_regular_file(p, ec)) return has_ext_ci(p, ".gam") ? fs::absolute(p, ec) : fs::path();
    if (fs::is_directory(p, ec)) return gam_in_folder(p);
    for (const auto &root : game_roots(argv0)) {                  // a bare name: <root>/<name>/*.gam, <root>/<name>.gam
        fs::path d = find_ci(root, arg);
        if (!d.empty() && fs::is_directory(d, ec)) { fs::path g = gam_in_folder(d); if (!g.empty()) return g; }
        fs::path f = find_ci(root, arg + ".gam");
        if (!f.empty()) return f;
    }
    return {};
}

static void list_games(const char *argv0)
{
    std::error_code ec;
    int n = 0;
    for (const auto &root : game_roots(argv0)) {
        std::vector<fs::path> found;
        for (const auto &e : fs::directory_iterator(root, ec))
            if (e.is_directory(ec)) { fs::path g = gam_in_folder(e.path()); if (!g.empty()) found.push_back(g); }
        std::sort(found.begin(), found.end());
        for (const auto &g : found) { printf("%-12s %s\n", lower(g.parent_path().filename().string()).c_str(), fs::absolute(g, ec).string().c_str()); n++; }
    }
    if (!n) printf("No games found. Pass a .gam file or a game folder, or set GM_GAMES to the folder that holds the game folders.\n");
}

static void usage()
{
    printf(
        "GameMaker player (1994 engine on raylib)\n"
        "\n"
        "usage: gmplay [options] [game]\n"
        "  game            a .gam file, a game folder, or a game name (see --list); without one the\n"
        "                  original menu appears and asks for a game\n"
        "\n"
        "options:\n"
        "  --fullscreen    start in full screen (--windowed for a window); remembered\n"
        "  --scale=MODE    int43 (default), fit43, intsq or fitsq: whole-number or fitted picture, 4:3 or\n"
        "                  square pixels; remembered\n"
        "  --volume=N      master volume 0-100;  --mute / --unmute\n"
        "  --no-gamepad    ignore gamepads\n"
        "  --list          list the games that can be found, then exit\n"
        "  --help          this text\n"
        "\n"
        "keys while playing:\n"
        "  F11 or Alt+Enter   full screen / window         F12              picture scaling\n"
        "  Alt+Up / Alt+Down  volume                        Alt+M            mute\n"
        "gamepad: the D-pad or left stick moves (a joystick in games, the arrow keys in menus), A = Enter,\n"
        "  B = Esc in menus, Start = Esc; in a game A and B are the joystick buttons.\n"
        "\n"
        "game folders searched for names: $GM_GAMES, <exe>/../../cd/gameware, <exe>/games, the current folder.\n"
        "settings file: %s\n",
#ifdef _WIN32
        "%APPDATA%\\gmplay\\gmplay.ini"
#elif defined(__APPLE__)
        "~/Library/Application Support/gmplay/gmplay.ini"
#else
        "~/.config/gmplay/gmplay.ini"
#endif
    );
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#ifdef GM_PLAYER_MODULE
extern "C" {
#include "fb.h"
}
extern "C" void gm_reload_config(void);   // GENC.C
extern "C" void gm_save_config(void);
#endif
// In the browser the module stays alive after the engine finishes (-sEXIT_RUNTIME=0), so that the page keeps working:
// saving the last scores, showing the end screen, reacting to keys that arrive late. The page is told here instead.
static void page_notify(int code)
{
    static bool done;
    if (done) return;
    done = true;
    gm_audio_shutdown();                              // a WebAudio node must not keep calling into a finished game
#ifdef GM_PLAYER_MODULE
    // The player as one more program of the design tools page (web/gmedit-web.js): same convention as the editors
    // (gmedit_main.cpp), the window is closed and the page is told by gmProgramExited.
    gm_save_config();
    fb_close();
    EM_ASM({ if (window.gmProgramExited) window.gmProgramExited($0); }, code);
#else
    EM_ASM({ if (window.gmEngineExited) window.gmEngineExited($0); }, code);
#endif
}
#endif

int main(int argc, char *argv[])
{
#ifdef _WIN32
    SetUnhandledExceptionFilter(crash_filter);
    _set_invalid_parameter_handler(invalid_parameter);
    if (getenv("GM_EXITTRACE")) atexit(trace_exit);
    DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &main_thread, 0, FALSE, DUPLICATE_SAME_ACCESS);
    if (getenv("GM_WATCHDOG")) CreateThread(NULL, 0, watchdog, NULL, 0, NULL);
#elif !defined(__EMSCRIPTEN__)
    for (int sig : { SIGSEGV, SIGBUS, SIGFPE, SIGILL, SIGABRT }) signal(sig, crash_handler);
    if (getenv("GM_EXITTRACE")) atexit(trace_exit);
    if (getenv("GM_WATCHDOG")) {
        main_thread = pthread_self();
        signal(SIGUSR2, hang_handler);
        pthread_t t;
        pthread_create(&t, NULL, watchdog, NULL);
    }
#endif

    gm_settings_load();

    // Options (anything starting with "--") are ours; the rest goes to the engine as it always did
    // ("gmplay demo.rec game.gam", "gmplay config", ...).
    std::vector<std::string> positional;
    bool list = false;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if (a.rfind("--", 0) != 0) { positional.push_back(a); continue; }
        std::string v = a.find('=') != std::string::npos ? a.substr(a.find('=') + 1) : "";
        std::string k = a.substr(0, a.find('='));
        if (k == "--help" || k == "-h") { usage(); return 0; }
        else if (k == "--list") list = true;
        else if (k == "--fullscreen") gm_settings.fullscreen = 1;
        else if (k == "--windowed") gm_settings.fullscreen = 0;
        else if (k == "--mute") gm_settings.mute = 1;
        else if (k == "--unmute") gm_settings.mute = 0;
        else if (k == "--no-gamepad") gm_settings.gamepad = 0;
        else if (k == "--volume") gm_settings.volume = std::max(0, std::min(100, atoi(v.c_str())));
        else if (k == "--scale") {
            int m = -1;
            for (int s = 0; s < GM_SCALE_COUNT; s++) if (v == gm_scale_name(s)) m = s;
            if (m < 0) { fprintf(stderr, "gmplay: --scale must be int43, fit43, intsq or fitsq\n"); return 2; }
            gm_settings.scale_mode = m;
        } else { fprintf(stderr, "gmplay: unknown option %s (try --help)\n", a.c_str()); return 2; }
    }
    if (list) { list_games(argv[0]); return 0; }

    // One non-option argument that is not a recording or a keyword: a game to launch.
    std::vector<char *> args = { argv[0] };
    std::string launch_name;
    if (positional.size() == 1 && !has_ext_ci(positional[0], ".rec") && lower(positional[0]).rfind("config", 0) != 0 &&
        lower(positional[0]) != "showvideomemory") {
        fs::path gam = resolve_game(argv[0], positional[0]);
        if (gam.empty()) {
            fprintf(stderr, "gmplay: no game found for \"%s\" (try gmplay --list)\n", positional[0].c_str());
            return 1;
        }
        std::error_code ec;
        fs::current_path(gam.parent_path(), ec);
        if (ec) { fprintf(stderr, "gmplay: cannot enter %s\n", gam.parent_path().string().c_str()); return 1; }
        launch_name = gam.filename().string();
        args.push_back((char *)launch_name.c_str());
    } else {
        for (const auto &p : positional) args.push_back((char *)p.c_str());
    }
    args.push_back(nullptr);
#ifdef __EMSCRIPTEN__
    atexit([] { page_notify(0); });                  // the engine also ends through exit() on some paths
#ifdef GM_PLAYER_MODULE
    gm_reload_config();                              // the data folder is in place now (see GENC.C)
#endif
    int rc = (int)gm_main_entry((int32_t)args.size() - 1, args.data());
    page_notify(rc);
    return rc;
#else
    return gm_main_entry((int32_t)args.size() - 1, args.data());
#endif
}

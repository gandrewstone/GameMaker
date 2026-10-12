// gmlaunch: the native replacement of GM.EXE (code/GM/GM.ASM), the program that ties the design tools together.
//
// GM.EXE runs menu.exe, and then whatever program the previous one asks for by its exit code (quit, utility, palchos,
// blocedit, monedit, mapmaker, charedit, image, sndedit, grator, playgame, menu = 0..11); a program that exits with 0
// ends GameMaker. menu.exe is told which program ran last through its command line ("cOoL" and the letter 'A' + number),
// so that it comes back on the right entry. The tools are the gm<name> programs next to this one, playgame is gmplay.
//
// usage: gmlaunch [folder]     the folder holds the data (help files, game folders); default: the current folder
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <filesystem>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <process.h>
#else
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

enum { QUIT = 0, UTILITY, PALCHOS, BLOCEDIT, MONEDIT, MAPMAKER, CHAREDIT, IMAGE, SNDEDIT, GRATOR, PLAYGAME, MENU };
static const char *const programs[] = { "", "gmutility", "gmpalchos", "gmblocedit", "gmmonedit", "gmmapmaker",
                                        "gmcharedit", "gmimage", "gmsndedit", "gmgrator", "gmplay", "gmmenu" };

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
#endif
    return fs::absolute(fs::path(argv0), ec).parent_path();
}

// Runs one program and returns its exit code, or -1 if it could not be started.
static int run(const fs::path &exe, const std::vector<std::string> &args)
{
    std::vector<const char *> argv;
    for (const auto &a : args) argv.push_back(a.c_str());
    argv.push_back(nullptr);
#ifdef _WIN32
    intptr_t rc = _spawnv(_P_WAIT, exe.string().c_str(), argv.data());
    return (int)rc;
#else
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        execv(exe.string().c_str(), (char *const *)argv.data());
        _exit(127);
    }
    int status = 0;
    if (waitpid(pid, &status, 0) < 0) return -1;
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
#endif
}

int main(int argc, char *argv[])
{
    fs::path bin = exe_dir(argv[0]);
    if (argc > 1) {
        std::error_code ec;
        fs::current_path(argv[1], ec);
        if (ec) { fprintf(stderr, "gmlaunch: cannot enter %s\n", argv[1]); return 2; }
    }
#ifdef _WIN32
    const char *ext = ".exe";
#else
    const char *ext = "";
#endif
    auto exe_of = [&](int prog) { return bin / (std::string(programs[prog]) + ext); };

    // first the menu, which was last "run" after playgame
    int last = PLAYGAME;
    int next = run(exe_of(MENU), { programs[MENU], "cOoL", std::string(1, (char)('A' + last)) });
    while (next > QUIT && next <= MENU) {
        std::vector<std::string> args = { programs[next] };
        if (next == MENU) { args.push_back("cOoL"); args.push_back(std::string(1, (char)('A' + last))); }
        int ran = next;
        next = run(exe_of(ran), args);
        if (next < 0) { fprintf(stderr, "gmlaunch: unable to run %s (is it next to gmlaunch?)\n", programs[ran]); return 1; }
        if (ran != MENU) last = ran;         // after menu itself the program before it stays the "previous" one
    }
    return next < 0 ? 1 : 0;
}

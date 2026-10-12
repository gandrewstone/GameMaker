// Host main() of the editor programs (menu, utility, palchos, ...). The program itself is reached through
// gm_main_entry, generated per program (see cmake/Editors.cmake and edit_entry.h).
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

extern "C" int32_t gm_main_entry(int32_t argc, char **argv);

#ifdef _WIN32
// A crash prints a symbolised call stack (needs the PDB next to the exe), as the player does (gmplay_main.cpp).
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

static void invalid_parameter(const wchar_t *, const wchar_t *, const wchar_t *, unsigned, uintptr_t)
{
    init_symbols();
    fprintf(stderr, "\n*** invalid parameter passed to a C runtime function; call stack:\n");
    CONTEXT ctx = {};
    RtlCaptureContext(&ctx);
    print_stack(GetCurrentThread(), ctx);
    ExitProcess(4);
}
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include "audio.h"
#include "dosplat.h"
extern "C" {
#include "fb.h"
}
// The page (web/gmedit-web.js) runs one program after the other. A finished program tells it which one asked for next:
// the exit code (see dosplat.h, gm_exit). The module itself stays alive (-sEXIT_RUNTIME=0); its window and audio are
// closed here so that it stops listening to the page.
extern "C" void gm_reload_config(void);   // GENC.C
extern "C" void gm_save_config(void);

static void page_notify(void)
{
    static bool done;
    if (done) return;
    done = true;
    gm_save_config();                                // what ~ConfigData does at the end of a DOS program
    gm_audio_shutdown();
    fb_close();
    EM_ASM({ if (window.gmProgramExited) window.gmProgramExited($0); }, gm_exit_code);
}
#endif

int main(int argc, char *argv[])
{
#ifdef _WIN32
    SetUnhandledExceptionFilter(crash_filter);
    _set_invalid_parameter_handler(invalid_parameter);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
#ifdef __EMSCRIPTEN__
    gm_exit_hook = page_notify;                      // exit(next program) in the editors; atexit does not run with EXIT_RUNTIME=0
    gm_reload_config();                              // the data folder is in place now (see GENC.C)
    int rc = (int)gm_main_entry((int32_t)argc, argv);
    gm_exit_code = rc;
    page_notify();
    return rc;
#else
    return (int)gm_main_entry((int32_t)argc, argv);
#endif
}

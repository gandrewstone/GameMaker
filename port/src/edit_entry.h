// Entry point of an editor program. The editor's own main() is renamed gm_game_main by shim/gmcompat.h and its
// signature differs from program to program (void or QuitCodes result, 16-bit int argc). Each editor is built from a
// generated file that includes the program's main source and then this header, so the call is checked against the
// real definition. The editors end through exit(code): that is how one program tells the next one to run
// (see EDITORS_SURVEY.md, section 1).
#ifndef GM_EDIT_ENTRY_H
#define GM_EDIT_ENTRY_H

template <class R> struct GmEditRet {
    template <class F> static int run(F f) { return (int)f(); }
};
template <> struct GmEditRet<void> {
    template <class F> static int run(F f) { f(); return 0; }
};

extern "C" int32_t gm_main_entry(int32_t argc, char **argv)
{
    typedef decltype(gm_game_main((short)0, (char **)0)) Result;
    return (int32_t)GmEditRet<Result>::run([&] { return gm_game_main((short)argc, argv); });
}

#endif

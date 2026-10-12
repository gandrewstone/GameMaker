// Real main() for cmf2wav; the tool itself is compiled with the Borland shim, which renames main().
#include <stdint.h>
extern "C" int32_t cmf2wav_entry(int32_t argc, char **argv);
int main(int argc, char **argv) { return cmf2wav_entry(argc, argv); }

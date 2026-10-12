/* Renders a CMF song to a WAV file with the same code the game uses: the original SBMUSIC.C (CMF parsing and
 * the per-tick PlayIt()), the port's FM driver (sound_port.cpp) and the OPL2 emulator, driven by a manual clock
 * at the engine's 146 Hz tick rate instead of the wall clock.
 *
 *   cmf2wav song.cmf out.wav [seconds]        (default 30 s)
 *
 * Built with the same compatibility shim as the engine (16-bit int), hence the separate entry point.
 */
#include "gen.h"
#include "gmgen.h"
#include "jstick.h"
#include "sound.h"
#include "audio.h"

char WorkDir[MAXFILENAMELEN + 1] = "";

int MakeFileName(char *out, const char *path, const char *name, const char *ext)
  {
  strcpy(out, path);
  strcat(out, name);
  return 1;
  }

extern "C" int32_t cmf2wav_entry(int32_t argc, char **argv)
  {
  if (argc < 3)
    {
    fprintf(stderr, "usage: cmf2wav song.cmf out.wav [seconds]\n");
    return 2;
    }
  double seconds = argc > 3 ? atof(argv[3]) : 30.0;

  gm_audio_manual_clock(1);
  ConfigStruct cs;
  memset(&cs, 0, sizeof cs);
  cs.SndPort = 0x220;
  if (!(SoundCard(&cs) & CARDEXIST)) return 3;

  int instruments = 0;
  int err = InitMusic(&instruments, argv[1]);
  if (err)
    {
    fprintf(stderr, "InitMusic failed: %d\n", err);
    return 4;
    }

  FILE *out = fopen(argv[2], "wb");
  if (!out) return 5;
  uint8_t header[44] = { 0 };
  fwrite(header, 1, 44, out);                       // filled in at the end

  const double tick = 1.0 / 146.0;                  // HDRCLK: the timer interrupt rate the songs are timed for
  static int16_t buf[4096];
  uint32_t total = 0;
  for (double t = 0; t < seconds; t += tick)
    {
    PlayIt();                                       // what NewTimer does once per tick
    gm_audio_advance(tick);
    uint32_t n;
    while ((n = gm_audio_drain(buf, 4096)) > 0) { fwrite(buf, 2, n, out); total += n; }
    }

  const uint32_t rate = GM_AUDIO_RATE, byte_rate = rate * 2, data_bytes = total * 2, riff = 36 + data_bytes, fmt = 16;
  const uint16_t pcm = 1, ch = 1, align = 2, bits = 16;
  fseek(out, 0, SEEK_SET);
  fwrite("RIFF", 1, 4, out); fwrite(&riff, 4, 1, out); fwrite("WAVEfmt ", 1, 8, out);
  fwrite(&fmt, 4, 1, out); fwrite(&pcm, 2, 1, out); fwrite(&ch, 2, 1, out); fwrite(&rate, 4, 1, out);
  fwrite(&byte_rate, 4, 1, out); fwrite(&align, 2, 1, out); fwrite(&bits, 2, 1, out);
  fwrite("data", 1, 4, out); fwrite(&data_bytes, 4, 1, out);
  fclose(out);
  printf("%s: %d instruments, %.1f s rendered\n", argv[1], instruments, (double)total / GM_AUDIO_RATE);
  return 0;
  }

/* Engine-side sound for the port; replaces SOUNDC.C (Sound Blaster voice driver) and the Creative FM driver
 * library SBCL.LIB that SBMUSIC.C calls.
 *
 * SBMUSIC.C itself is compiled unchanged: it parses the CMF file and its PlayIt() runs once per timer tick
 * (146 Hz), turning song events into calls to the sbfd_* functions below. Those play the notes on an emulated
 * OPL2 (see audio.cpp), allocating the chip's nine voices to MIDI channels the way the Creative driver did.
 * Digital sound effects (.VOC files) are decoded and mixed by audio.cpp.
 */
#include "gen.h"
#include "gmgen.h"
#include "jstick.h"
#include "windio.h"
#include "sound.h"
#include "sbc.h"
#include "sbcmusic.h"
#include "audio.h"

/* ---------------------------------------------------------------------------------------------
 * Sound Blaster detection: always "present" with an FM chip and a DSP
 * ------------------------------------------------------------------------------------------- */
unsigned ct_io_addx = 0x220;
unsigned ct_int_num = 7;

extern "C" int sbc_check_card(void) { return 3; }      // bit 1 = FM chip, bit 0 = DSP
extern "C" int sbc_scan_int(void) { return 7; }

int DetectCard(SoundCards *s, unsigned int *Port, unsigned int *Interrupt, char *drvrname)
  {
  *s = SndBlaster;
  *Port = 0x220;
  *Interrupt = 7;
  return 1;
  }

/* ---------------------------------------------------------------------------------------------
 * FM driver (the part of the Creative driver the game used)
 * ------------------------------------------------------------------------------------------- */
namespace {

struct Voice
  {
  bool          on;
  int           ch, note;
  unsigned long age;
  unsigned char b0;          // register B0 contents without the key-on bit
  };

const unsigned char OP_MOD[9] = { 0, 1, 2, 8, 9, 10, 16, 17, 18 };   // modulator operator offset of each voice

Voice          voices[9];
unsigned long  age_counter;
const unsigned char *instruments;     // 16-byte entries
unsigned       instrument_count;
unsigned char  channel_program[16];
bool           rhythm_mode;
unsigned char  rhythm_bits;           // current key bits of register BD (without the mode bit)

const unsigned char DEFAULT_INSTRUMENT[16] = { 0x01, 0x01, 0x10, 0x00, 0xF0, 0xF0, 0x77, 0x77, 0, 0, 0x00, 0, 0, 0, 0, 0 };

bool log_events() { static int on = -1; if (on < 0) on = getenv("GM_SOUNDLOG") != NULL; return on != 0; }

void W(int reg, int value) { gm_opl_write((uint8_t)reg, (uint8_t)value); }

const unsigned char *instrument_for(int channel)
  {
  unsigned p = channel_program[channel & 15];
  if (instruments && p < instrument_count) return instruments + p * 16;
  return DEFAULT_INSTRUMENT;
  }

void note_to_freq(int note, int &fnum, int &block)
  {
  double f = 440.0 * pow(2.0, (note - 69) / 12.0);
  for (block = 0; block < 8; block++)
    {
    fnum = (int)(f * (double)(1 << (20 - block)) / 49716.0 + 0.5);
    if (fnum < 1024) return;
    }
  block = 7;
  fnum = 1023;
  }

int level_for(int base, int velocity)             // total level 0 (loud) .. 63, scaled by velocity
  {
  if (velocity > 127) velocity = 127;
  return 63 - ((63 - base) * velocity) / 127;
  }

void load_operator(int op, const unsigned char *i, bool carrier, int velocity, bool scale)
  {
  int k = carrier ? 1 : 0;
  int tl = i[2 + k] & 0x3F;
  W(0x20 + op, i[0 + k]);
  W(0x40 + op, (i[2 + k] & 0xC0) | (scale ? level_for(tl, velocity) : tl));
  W(0x60 + op, i[4 + k]);
  W(0x80 + op, i[6 + k]);
  W(0xE0 + op, i[8 + k]);
  }

void load_voice(int v, const unsigned char *i, int velocity)
  {
  bool additive = (i[10] & 1) != 0;
  load_operator(OP_MOD[v],     i, false, velocity, additive);   // a modulator only sets loudness in additive mode
  load_operator(OP_MOD[v] + 3, i, true,  velocity, true);
  W(0xC0 + v, i[10]);
  }

void key_off(int v)
  {
  W(0xB0 + v, voices[v].b0);
  voices[v].on = false;
  }

int usable_voices() { return rhythm_mode ? 6 : 9; }

void silence_all()
  {
  for (int v = 0; v < 9; v++) { W(0xB0 + v, 0); voices[v].on = false; voices[v].b0 = 0; }
  rhythm_bits = 0;
  W(0xBD, rhythm_mode ? 0x20 : 0x00);
  }

int allocate_voice(int ch, int note)
  {
  int n = usable_voices();
  for (int v = 0; v < n; v++)                       // same note again: retrigger the voice already playing it
    if (voices[v].on && voices[v].ch == ch && voices[v].note == note) return v;
  for (int v = 0; v < n; v++)
    if (!voices[v].on) return v;
  int oldest = 0;                                   // all busy: steal the voice that has been sounding longest
  for (int v = 1; v < n; v++)
    if (voices[v].age < voices[oldest].age) oldest = v;
  return oldest;
  }

// Percussion in rhythm mode: MIDI channels 11..15 are bass drum, snare, tom-tom, cymbal, hi-hat.
struct Drum { unsigned char bit; int voice; int op; bool carrier; };
const Drum DRUMS[5] = {
  { 0x10, 6, 16, false },     // bass drum: uses both operators of voice 6 (handled specially)
  { 0x08, 7, 20, true  },     // snare drum: carrier of voice 7
  { 0x04, 8, 18, false },     // tom-tom: modulator of voice 8
  { 0x02, 8, 21, true  },     // cymbal: carrier of voice 8
  { 0x01, 7, 17, false },     // hi-hat: modulator of voice 7
};

void drum_note(int ch, int note, int velocity, bool on)
  {
  const Drum &d = DRUMS[ch - 11];
  if (!on) { rhythm_bits &= (unsigned char)~d.bit; W(0xBD, 0x20 | rhythm_bits); return; }

  const unsigned char *i = instrument_for(ch);
  if (ch == 11) load_voice(6, i, velocity);
  else          load_operator(d.op, i, d.carrier, velocity, true);

  int fnum, block;
  note_to_freq(note, fnum, block);
  W(0xA0 + d.voice, fnum & 0xFF);
  W(0xB0 + d.voice, (block << 2) | (fnum >> 8));
  rhythm_bits &= (unsigned char)~d.bit;             // retrigger
  W(0xBD, 0x20 | rhythm_bits);
  rhythm_bits |= d.bit;
  W(0xBD, 0x20 | rhythm_bits);
  }

}  // namespace

extern "C" int sbfd_init(void)
  {
  W(0x01, 0x20);                                    // enable the waveform-select registers
  W(0x08, 0x00);
  rhythm_mode = false;
  memset(channel_program, 0, sizeof channel_program);
  silence_all();
  return 0;
  }

extern "C" int sbfd_instrument(char far *table)
  {
  if (log_events()) fprintf(stderr, "sbfd instruments=%p size=%u\n", (void *)table, table ? gm_farsize(table) : 0);
  instruments = (const unsigned char *)table;
  instrument_count = table ? gm_farsize(table) / 16 : 0;
  return 0;
  }

extern "C" int sbfd_reset(void)
  {
  gm_opl_reset();
  return sbfd_init();
  }

extern "C" int sbfd_music_off(void)
  {
  for (int v = 0; v < 9; v++)
    if (voices[v].on) key_off(v);
  if (rhythm_mode) { rhythm_bits = 0; W(0xBD, 0x20); }
  return 0;
  }

extern "C" int sbfd_setmode(char mode)
  {
  if (log_events()) fprintf(stderr, "sbfd setmode %d\n", mode);
  sbfd_music_off();
  rhythm_mode = mode != 0;
  rhythm_bits = 0;
  W(0xBD, rhythm_mode ? 0x20 : 0x00);
  return 0;
  }

extern "C" void sbfd_program_change(char channel, char program)
  {
  channel_program[channel & 15] = (unsigned char)program;
  }

extern "C" int sbfd_note_on(char channel, char note, char velocity)
  {
  if (log_events()) fprintf(stderr, "sbfd note_on ch=%d note=%d vel=%d prog=%d\n", channel, note, velocity, channel_program[channel & 15]);
  int ch = channel & 15;
  if (rhythm_mode && ch >= 11) { drum_note(ch, note, velocity, true); return 0; }

  int v = allocate_voice(ch, note);
  W(0xB0 + v, 0x00);                                // restart the envelope
  load_voice(v, instrument_for(ch), velocity);
  int fnum, block;
  note_to_freq(note, fnum, block);
  voices[v].b0 = (unsigned char)((block << 2) | (fnum >> 8));
  W(0xA0 + v, fnum & 0xFF);
  W(0xB0 + v, 0x20 | voices[v].b0);
  voices[v].on = true;
  voices[v].ch = ch;
  voices[v].note = note;
  voices[v].age = ++age_counter;
  return 0;
  }

extern "C" int sbfd_note_off(char channel, char note, char velocity)
  {
  int ch = channel & 15;
  if (rhythm_mode && ch >= 11) { drum_note(ch, note, 0, false); return 0; }
  for (int v = 0; v < usable_voices(); v++)
    if (voices[v].on && voices[v].ch == ch && voices[v].note == note) { key_off(v); break; }
  return 0;
  }

/* ---------------------------------------------------------------------------------------------
 * Digital sound effects (.VOC)
 * ------------------------------------------------------------------------------------------- */
static boolean driverloaded = FALSE;

char InitSbVocDriver(ConfigStruct *cs)
  {
  driverloaded = TRUE;
  return TRUE;
  }

char PlaySbVocFile(char far *filename)
  {
  char fullfilename[MAXFILENAMELEN];
  if (!driverloaded) return FALSE;

  MakeFileName(fullfilename, WorkDir, filename, ".voc");
  FILE *fp = fopen(fullfilename, "rb");
  if (!fp) return FALSE;
  fseek(fp, 0L, SEEK_END);
  long size = ftell(fp);
  fseek(fp, 0L, SEEK_SET);
  unsigned char *buf = (unsigned char *)malloc((size_t)size);
  bool ok = buf && fread(buf, 1, (size_t)size, fp) == (size_t)size;
  fclose(fp);
  int rc = ok ? gm_voc_play(buf, (uint32_t)size) : 1;
  free(buf);
  if (log_events()) fprintf(stderr, "voc %s -> %d\n", fullfilename, rc);
  return rc == 0 ? TRUE : FALSE;
  }

void ShutSbVocDriver(void)
  {
  driverloaded = FALSE;
  gm_pcm_stop();
  }

void StopSound(void) { gm_pcm_stop(); }
void FreeSoundSample(void) { gm_pcm_stop(); }

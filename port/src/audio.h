/* Sound for the port: an OPL2 (Sound Blaster FM) synthesizer, the PC speaker, and one digital sample voice,
 * mixed into a single raylib audio stream.
 *
 * Timing: register writes and speaker changes are applied at the moment they happen (the mixer first renders
 * samples up to "now" on the wall clock), so music keeps the engine's 146 Hz tick timing instead of being
 * quantised to audio buffers. Everything runs on the caller's thread; gm_audio_pump() moves finished samples
 * to the device and must be called often (gm_pump() does).
 *
 * Fixed-width types only: this header is included both by plain code and, through the shim, by engine code
 * where `int` is 16 bits wide.
 */
#ifndef GM_AUDIO_H
#define GM_AUDIO_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GM_AUDIO_RATE 49716          /* native YM3812 output rate (3.579545 MHz / 72) */

void gm_audio_pump(void);                           /* render up to now and feed the audio device */

/* FM chip (OPL2) */
void gm_opl_write(uint8_t reg, uint8_t value);
void gm_opl_reset(void);

/* PC speaker: square wave at `hz`, 0 = off */
void gm_speaker(uint16_t hz);

/* Digital sample (Creative Voice File image). Replaces any sample still playing. Returns 0 on success. */
int  gm_voc_play(const uint8_t *data, uint32_t size);
void gm_pcm_stop(void);

/* Master volume (0..100, default 100) and mute. They scale what goes to the audio device only; a GM_WAV capture
 * always holds the unscaled mix. */
void gm_audio_set_volume(int pct);
int  gm_audio_volume(void);
void gm_audio_set_mute(int on);
int  gm_audio_muted(void);
void gm_audio_set_headless(int on);       /* never open an audio device (GM_HEADLESS test mode); GM_WAV still captures */
void gm_audio_stats(unsigned *fed, unsigned *starved);   /* chunks sent to the device / times it wanted one and none was ready */
void gm_audio_shutdown(void);               /* close the audio device (end of a web page session) */

/* Offline use (tests, tools): drive the mixer from a manual clock and collect samples instead of using a device. */
void     gm_audio_manual_clock(int on);
void     gm_audio_advance(double seconds);
uint32_t gm_audio_drain(int16_t *dst, uint32_t max);

#ifdef __cplusplus
}
#endif
#endif

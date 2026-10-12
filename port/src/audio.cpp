/* Mixer and synthesis for the port's sound. See audio.h. */
#include "audio.h"
#include "ymfm_opl.h"
#include <raylib.h>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {

struct Interface : ymfm::ymfm_interface {};

Interface g_iface;
ymfm::ym3812 *g_opl;

/* ---- clock ---------------------------------------------------------------------------------------- */
bool g_manual;
double g_manual_t;
std::chrono::steady_clock::time_point g_t0;
bool g_t0_set;

double now_seconds()
{
    if (g_manual) return g_manual_t;
    auto t = std::chrono::steady_clock::now();
    if (!g_t0_set) { g_t0 = t; g_t0_set = true; }
    return std::chrono::duration<double>(t - g_t0).count();
}

/* ---- finished samples waiting for the device ------------------------------------------------------ */
constexpr uint32_t RING = 1u << 15;
int16_t g_ring[RING];
uint64_t g_rd, g_wr;                 /* monotonic counts; index = count % RING */

int  g_volume = 100;                 /* master volume 0..100, applied where samples go to the device */
bool g_mute;

uint64_t g_synth_pos;                /* samples synthesised so far */
bool g_device_ok;                    /* a real output device is playing (set in ensure_device) */

/* Samples the synthesis clock runs ahead of the wall clock. With a real device this must exceed what the device holds
 * (two 1024-sample buffers): when the ring is shorter the device is fed partial chunks, which cuts continuous sound
 * (music) into pieces and makes it sound slow. Tests and captures keep the small value. */
constexpr double LATENCY_TEST = 1536;
#ifdef __EMSCRIPTEN__
#define GM_CHUNK 2048
constexpr double LATENCY_DEVICE = 3 * GM_CHUNK;   /* the ring must hold a whole chunk even right after the device took two */
#else
#define GM_CHUNK 1024
constexpr double LATENCY_DEVICE = LATENCY_TEST;
#endif
constexpr uint32_t MAX_CATCHUP = 8192;

/* ---- voices ---------------------------------------------------------------------------------------- */
uint16_t g_spk_hz;
double g_spk_phase;
std::vector<int16_t> g_pcm;          /* current sample, already at GM_AUDIO_RATE */
size_t g_pcm_pos;

void ensure_opl()
{
    if (g_opl) return;
    g_opl = new ymfm::ym3812(g_iface);
    g_opl->reset();
}

int16_t clamp16(int32_t v) { return (int16_t)(v > 32767 ? 32767 : v < -32768 ? -32768 : v); }

int16_t next_sample()
{
    ymfm::ym3812::output_data out;
    g_opl->generate(&out, 1);
    int32_t mix = (out.data[0] * 3) / 2;                     /* the chip's 12-bit range, brought up to a normal level */
    if (g_spk_hz) {
        g_spk_phase += (double)g_spk_hz / GM_AUDIO_RATE;
        g_spk_phase -= std::floor(g_spk_phase);
        mix += g_spk_phase < 0.5 ? 2600 : -2600;          /* the PC speaker is quiet next to FM */
    }
    if (g_pcm_pos < g_pcm.size()) mix += (g_pcm[g_pcm_pos++] * 7) / 10;     /* headroom so a loud sample over music does not clip */
    return clamp16(mix);
}

/* synthesise until the synthesis clock reaches the wall clock (plus latency) */
void render_to_now()
{
    ensure_opl();
    double target = now_seconds() * GM_AUDIO_RATE + (g_device_ok ? LATENCY_DEVICE : LATENCY_TEST);
    if (target - (double)g_synth_pos > MAX_CATCHUP) g_synth_pos = (uint64_t)target - 2048;   /* stalled: skip ahead */
    while ((double)g_synth_pos < target && g_wr - g_rd < RING) {
        g_ring[g_wr++ % RING] = next_sample();
        g_synth_pos++;
    }
}

/* ---- optional capture: GM_WAV=<file> writes everything sent to the device as a 16-bit mono WAV ----- */
FILE *g_wav;
uint32_t g_wav_bytes;
bool g_wav_tried;

void wav_close()
{
    if (!g_wav) return;
    uint32_t v;
    std::fseek(g_wav, 4, SEEK_SET);  v = 36 + g_wav_bytes; std::fwrite(&v, 4, 1, g_wav);
    std::fseek(g_wav, 40, SEEK_SET); v = g_wav_bytes;      std::fwrite(&v, 4, 1, g_wav);
    std::fclose(g_wav);
    g_wav = nullptr;
}

bool wav_open()
{
    if (!g_wav_tried) {
        g_wav_tried = true;
        const char *path = std::getenv("GM_WAV");
        if (path && (g_wav = std::fopen(path, "wb"))) {
            const uint32_t rate = GM_AUDIO_RATE, byte_rate = GM_AUDIO_RATE * 2, fmt_len = 16;
            const uint16_t pcm = 1, ch = 1, align = 2, bits = 16;
            const uint32_t zero = 0;
            std::fwrite("RIFF", 1, 4, g_wav); std::fwrite(&zero, 4, 1, g_wav); std::fwrite("WAVEfmt ", 1, 8, g_wav);
            std::fwrite(&fmt_len, 4, 1, g_wav); std::fwrite(&pcm, 2, 1, g_wav); std::fwrite(&ch, 2, 1, g_wav);
            std::fwrite(&rate, 4, 1, g_wav); std::fwrite(&byte_rate, 4, 1, g_wav);
            std::fwrite(&align, 2, 1, g_wav); std::fwrite(&bits, 2, 1, g_wav);
            std::fwrite("data", 1, 4, g_wav); std::fwrite(&zero, 4, 1, g_wav);
            std::atexit(wav_close);
        }
    }
    return g_wav != nullptr;
}

void wav_write(const int16_t *s, uint32_t n)
{
    if (!wav_open()) return;
    std::fwrite(s, sizeof(int16_t), n, g_wav);
    g_wav_bytes += n * 2;
}

/* ---- raylib device --------------------------------------------------------------------------------- */
AudioStream g_stream;
bool g_device_tried, g_headless;
uint32_t g_fed, g_starved;           /* chunks sent to the device / times it wanted one and the ring was short (GM_SOUNDLOG) */

/* The size of one device buffer (the stream has two). In the browser the audio callback asks for far more than 2 x 1024
 * samples at a time and the rest of each call would be silence, so the buffers are larger there (see LATENCY_DEVICE). */
constexpr int CHUNK = GM_CHUNK;

void ensure_device()
{
    if (g_device_tried) return;
    g_device_tried = true;
    if (g_headless) return;                       /* test mode: samples are only captured (GM_WAV) */
    SetTraceLogLevel(LOG_WARNING);
    InitAudioDevice();
    if (!IsAudioDeviceReady()) { std::fprintf(stderr, "audio: no output device, running silent\n"); return; }
    SetAudioStreamBufferSizeDefault(CHUNK);
    g_stream = LoadAudioStream(GM_AUDIO_RATE, 16, 1);
    PlayAudioStream(g_stream);
    g_device_ok = true;
}

/* ---- Creative Voice File ------------------------------------------------------------------------ */
void append_resampled(std::vector<int16_t> &dst, const std::vector<int16_t> &src, double rate)
{
    if (src.empty() || rate <= 0) return;
    size_t n = (size_t)((double)src.size() * GM_AUDIO_RATE / rate);
    double step = rate / GM_AUDIO_RATE;
    for (size_t i = 0; i < n; i++) {
        double p = i * step;
        size_t k = (size_t)p;
        double f = p - (double)k;
        int32_t a = src[k < src.size() ? k : src.size() - 1];
        int32_t b = src[k + 1 < src.size() ? k + 1 : src.size() - 1];
        dst.push_back((int16_t)(a + (b - a) * f));
    }
}

/* Creative's 4-bit ADPCM: a reference byte, then two 4-bit codes per byte (high nibble first) */
void decode_adpcm4(const uint8_t *p, size_t n, std::vector<int16_t> &out)
{
    static const int8_t scale_map[64] = {
        0, 1, 2, 3, 4, 5, 6, 7, 0, -1, -2, -3, -4, -5, -6, -7,
        1, 3, 5, 7, 9, 11, 13, 15, -1, -3, -5, -7, -9, -11, -13, -15,
        2, 6, 10, 14, 18, 22, 26, 30, -2, -6, -10, -14, -18, -22, -26, -30,
        4, 12, 20, 28, 36, 44, 52, 60, -4, -12, -20, -28, -36, -44, -52, -60 };
    static const uint8_t adjust_map[64] = {
        0, 0, 0, 0, 0, 16, 16, 16, 0, 0, 0, 0, 0, 0, 0, 0,
        240, 0, 0, 0, 0, 16, 16, 16, 240, 0, 0, 0, 0, 0, 0, 0,
        240, 0, 0, 0, 0, 16, 16, 16, 240, 0, 0, 0, 0, 0, 0, 0,
        240, 0, 0, 0, 0, 0, 0, 0, 240, 0, 0, 0, 0, 0, 0, 0 };
    if (n < 1) return;
    int ref = p[0];
    int scale = 0;
    out.push_back((int16_t)((ref - 128) << 8));
    for (size_t i = 1; i < n; i++)
        for (int half = 0; half < 2; half++) {
            int code = half == 0 ? p[i] >> 4 : p[i] & 15;
            int idx = code + scale;
            if (idx >= 0 && idx <= 63) {
                ref += scale_map[idx];
                ref = ref < 0 ? 0 : ref > 255 ? 255 : ref;
                scale = (scale + adjust_map[idx]) & 0xFF;
            }
            out.push_back((int16_t)((ref - 128) << 8));
        }
}

}  // namespace

extern "C" {

void gm_audio_manual_clock(int on) { g_manual = on != 0; g_manual_t = 0; }
void gm_audio_advance(double seconds) { g_manual_t += seconds; render_to_now(); }

uint32_t gm_audio_drain(int16_t *dst, uint32_t max)
{
    uint32_t n = 0;
    while (n < max && g_rd < g_wr) dst[n++] = g_ring[g_rd++ % RING];
    return n;
}

void gm_audio_pump(void)
{
    if (!g_manual) ensure_device();
    render_to_now();
    if (!g_device_ok && !wav_open()) { g_rd = g_wr; return; }   /* nobody listening: don't let the ring fill up */
    if (!g_device_ok) {                                          /* capture only */
        int16_t chunk[CHUNK];
        while (g_wr - g_rd >= CHUNK) { gm_audio_drain(chunk, CHUNK); wav_write(chunk, CHUNK); }
        return;
    }
    while (IsAudioStreamProcessed(g_stream)) {
        int16_t chunk[CHUNK];
#ifdef __EMSCRIPTEN__
        if (g_wr - g_rd < CHUNK) { g_starved++; break; }          /* only whole chunks: padding with silence cuts the music */
        g_fed++;
        gm_audio_drain(chunk, CHUNK);
#else
        uint32_t got = gm_audio_drain(chunk, CHUNK);
        if (got < CHUNK) std::memset(chunk + got, 0, (CHUNK - got) * sizeof(int16_t));   /* underrun: silence */
#endif
        wav_write(chunk, CHUNK);                                  /* captures keep the unscaled mix */
        float g = g_mute ? 0.0f : (float)(g_volume * g_volume) / 10000.0f;   /* squared: loudness follows the slider */
        if (g != 1.0f) for (uint32_t i = 0; i < CHUNK; i++) chunk[i] = (int16_t)(chunk[i] * g);
        UpdateAudioStream(g_stream, chunk, CHUNK);
    }
}

/* Closes the audio device (the browser page does this when the game ends: a WebAudio node keeps calling back into
 * the module otherwise). Silent afterwards. */
void gm_audio_shutdown(void)
{
    if (!g_device_ok) return;
    g_device_ok = false;
    if (std::getenv("GM_SOUNDLOG")) std::fprintf(stderr, "audio: %u chunks fed, %u times starved\n", g_fed, g_starved);
    UnloadAudioStream(g_stream);
    CloseAudioDevice();
}

void gm_audio_set_headless(int on) { g_headless = on != 0; }
void gm_audio_stats(unsigned *fed, unsigned *starved) { *fed = g_fed; *starved = g_starved; }
void gm_audio_set_volume(int pct) { g_volume = pct < 0 ? 0 : (pct > 100 ? 100 : pct); }
int  gm_audio_volume(void) { return g_volume; }
void gm_audio_set_mute(int on) { g_mute = on != 0; }
int  gm_audio_muted(void) { return g_mute; }

void gm_opl_write(uint8_t reg, uint8_t value)
{
    render_to_now();                       /* everything before this moment sounds as before */
    g_opl->write_address(reg);
    g_opl->write_data(value);
}

void gm_opl_reset(void)
{
    ensure_opl();
    g_opl->reset();
}

void gm_speaker(uint16_t hz)
{
    if (hz == g_spk_hz) return;
    render_to_now();
    g_spk_hz = hz;
}

void gm_pcm_stop(void)
{
    render_to_now();
    g_pcm.clear();
    g_pcm_pos = 0;
}

int gm_voc_play(const uint8_t *d, uint32_t size)
{
    if (size < 26 || std::memcmp(d, "Creative Voice File", 19) != 0) return 1;
    std::vector<int16_t> out;
    size_t p = d[20] | (d[21] << 8);                    /* header size */
    double rate = 8000;
    int codec = 0;
    double ext_rate = 0;                                /* from a type-8 block, applies to the next type-1 block */
    size_t loop_start = 0;
    int loop_count = 0;
    bool in_loop = false;

    while (p + 4 <= size && d[p] != 0) {
        int type = d[p];
        size_t len = d[p + 1] | (d[p + 2] << 8) | (d[p + 3] << 16);
        const uint8_t *b = d + p + 4;
        if (p + 4 + len > size) len = size - p - 4;
        std::vector<int16_t> seg;
        switch (type) {
        case 1:                                         /* sound data: freq divisor, codec, data */
            if (len < 2) break;
            rate = ext_rate > 0 ? ext_rate : 1000000.0 / (256 - b[0]);
            ext_rate = 0;
            codec = b[1];
            [[fallthrough]];
        case 2:                                         /* continuation of the last sound data block */
        {
            const uint8_t *s = type == 1 ? b + 2 : b;
            size_t n = type == 1 ? len - 2 : len;
            if (codec == 0) for (size_t i = 0; i < n; i++) seg.push_back((int16_t)((s[i] - 128) << 8));
            else if (codec == 1) decode_adpcm4(s, n, seg);
            else seg.assign(n * (codec == 3 ? 4 : 3), 0);   /* 2-bit and 2.6-bit ADPCM: not decoded, kept as silence */
            append_resampled(out, seg, rate);
            break;
        }
        case 3:                                         /* silence: length (count-1), freq divisor */
            if (len >= 3) {
                double sr = 1000000.0 / (256 - b[2]);
                seg.assign((size_t)((b[0] | (b[1] << 8)) + 1), 0);
                append_resampled(out, seg, sr);
            }
            break;
        case 6:                                         /* repeat start */
            if (len >= 2) { loop_count = b[0] | (b[1] << 8); loop_start = out.size(); in_loop = true; }
            break;
        case 7:                                         /* repeat end: play the section again `count` more times (endless -> once) */
            if (in_loop) {
                std::vector<int16_t> section(out.begin() + (long)loop_start, out.end());
                int times = loop_count == 0xFFFF ? 1 : loop_count;
                for (int i = 0; i < times && out.size() < (size_t)GM_AUDIO_RATE * 60; i++) out.insert(out.end(), section.begin(), section.end());
                in_loop = false;
            }
            break;
        case 8:                                         /* extra info: rate for the next sound data block */
            if (len >= 4) {
                int fd = b[0] | (b[1] << 8), mode = b[3];
                if (fd != 65536) ext_rate = 256000000.0 / ((mode + 1) * (65536 - fd));
            }
            break;
        case 9:                                         /* new format: 32-bit rate, bits, channels, codec */
            if (len >= 12) {
                double sr = b[0] | (b[1] << 8) | (b[2] << 16) | ((uint32_t)b[3] << 24);
                int bits = b[4], ch = b[5] ? b[5] : 1, cd = b[6] | (b[7] << 8);
                const uint8_t *s = b + 12;
                size_t n = len - 12;
                if (cd == 0 && bits == 8) for (size_t i = 0; i + ch <= n; i += ch) seg.push_back((int16_t)((s[i] - 128) << 8));
                else if (cd == 4 && bits == 16) for (size_t i = 0; i + 2 * ch <= n; i += 2 * ch) seg.push_back((int16_t)(s[i] | (s[i + 1] << 8)));
                append_resampled(out, seg, sr);
            }
            break;
        default: break;                                 /* 4 marker, 5 text, ... */
        }
        p += 4 + len;
    }

    render_to_now();
    g_pcm.swap(out);
    g_pcm_pos = 0;
    return g_pcm.empty() ? 1 : 0;
}

}  // extern "C"

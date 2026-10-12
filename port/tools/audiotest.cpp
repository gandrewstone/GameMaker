// Self-checking test of the mixer (audio.cpp): PC speaker pitch, an OPL2 note written straight to the chip,
// and a Creative Voice File. Runs on the manual clock, so it needs no audio device. Exit code 0 = pass.
#include "audio.h"
#include <cmath>
#include <cstdio>
#include <vector>

static std::vector<int16_t> render(double seconds)
{
    std::vector<int16_t> out;
    static int16_t buf[4096];
    for (double t = 0; t < seconds; t += 0.01) {
        gm_audio_advance(0.01);
        uint32_t n;
        while ((n = gm_audio_drain(buf, 4096)) > 0) out.insert(out.end(), buf, buf + n);
    }
    return out;
}

// frequency from rising zero crossings of the middle part of the signal
static double pitch(const std::vector<int16_t> &s)
{
    size_t a = s.size() / 4, b = s.size() * 3 / 4;
    int crossings = 0;
    for (size_t i = a + 1; i < b; i++) if (s[i - 1] < 0 && s[i] >= 0) crossings++;
    return crossings / ((double)(b - a) / GM_AUDIO_RATE);
}

static int failures;
static void check(const char *what, bool ok, double value)
{
    std::printf("%-44s %s (%.1f)\n", what, ok ? "ok  " : "FAIL", value);
    if (!ok) failures++;
}

int main()
{
    gm_audio_manual_clock(1);

    gm_speaker(440);
    double f = pitch(render(0.5));
    gm_speaker(0);
    check("PC speaker at 440 Hz", std::fabs(f - 440) < 440 * 0.02, f);
    auto quiet = render(0.1);
    int peak = 0;
    for (int16_t v : quiet) peak = std::abs(v) > peak ? std::abs(v) : peak;
    check("speaker off is silent (peak)", peak < 50, peak);

    // OPL2: channel 0, sine carrier (modulator muted), A4 = 440 Hz: block 4, fnum 0x241 (= 440 * 2^16 / 49716)
    gm_opl_reset();
    const uint8_t regs[][2] = { {0x01, 0x20}, {0x20, 0x21}, {0x23, 0x21}, {0x40, 0x3F}, {0x43, 0x00}, {0x60, 0xF0}, {0x63, 0xF0},
                                {0x80, 0x0F}, {0x83, 0x0F}, {0xC0, 0x00}, {0xA0, 0x41}, {0xB0, 0x32} };
    for (auto &r : regs) gm_opl_write(r[0], r[1]);
    f = pitch(render(0.5));
    check("OPL2 note A4 (block 4, fnum 0x241)", std::fabs(f - 440) < 440 * 0.02, f);
    gm_opl_write(0xB0, 0x12);                                      // key off
    render(0.5);
    quiet = render(0.1);
    peak = 0;
    for (int16_t v : quiet) peak = std::abs(v) > peak ? std::abs(v) : peak;
    check("OPL2 key off decays to silence (peak)", peak < 200, peak);

    // VOC: header, then one sound-data block: 8000 Hz, 8-bit PCM, 4000 samples = 0.5 s of a 1 kHz square wave
    std::vector<uint8_t> voc = { 'C','r','e','a','t','i','v','e',' ','V','o','i','c','e',' ','F','i','l','e',0x1A, 26, 0, 0x0A, 0x01, 0x29, 0x11 };
    const int samples = 4000, len = samples + 2;
    voc.push_back(1); voc.push_back(len & 255); voc.push_back((len >> 8) & 255); voc.push_back(0);
    voc.push_back(256 - 1000000 / 8000); voc.push_back(0);
    for (int i = 0; i < samples; i++) voc.push_back((i / 4) % 2 ? 0xC0 : 0x40);
    voc.push_back(0);
    check("VOC accepted", gm_voc_play(voc.data(), (uint32_t)voc.size()) == 0, 0);
    auto pcm = render(0.7);
    size_t active = 0;
    for (int16_t v : pcm) if (std::abs(v) > 500) active++;
    double secs = (double)active / GM_AUDIO_RATE;
    check("VOC plays for 0.5 s", std::fabs(secs - 0.5) < 0.03, secs);
    f = pitch(std::vector<int16_t>(pcm.begin(), pcm.begin() + (long)(0.5 * GM_AUDIO_RATE)));
    check("VOC square wave pitch (1 kHz)", std::fabs(f - 1000) < 40, f);

    std::printf(failures ? "%d FAILED\n" : "all passed\n", failures);
    return failures ? 1 : 0;
}

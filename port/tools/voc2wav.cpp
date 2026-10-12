// Decodes a Creative Voice File with the port's mixer (audio.cpp) and writes it as a WAV, reporting its length.
//   voc2wav in.voc out.wav
#include "audio.h"
#include <cstdio>
#include <cstring>
#include <vector>

int main(int argc, char **argv)
{
    if (argc < 3) { std::fprintf(stderr, "usage: voc2wav in.voc out.wav\n"); return 2; }
    FILE *f = std::fopen(argv[1], "rb");
    if (!f) { std::perror(argv[1]); return 1; }
    std::fseek(f, 0, SEEK_END); long n = std::ftell(f); std::fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> data(n);
    if (std::fread(data.data(), 1, n, f) != (size_t)n) return 1;
    std::fclose(f);

    gm_audio_manual_clock(1);
    if (gm_voc_play(data.data(), (uint32_t)n) != 0) { std::fprintf(stderr, "%s: not playable\n", argv[1]); return 3; }

    // play it out on the manual clock: stop after 2 s of silence or 60 s
    std::vector<int16_t> pcm;
    static int16_t buf[4096];
    int quiet = 0;
    for (double t = 0; t < 60.0 && quiet < 100; t += 0.02) {
        gm_audio_advance(0.02);
        uint32_t got;
        while ((got = gm_audio_drain(buf, 4096)) > 0) {
            for (uint32_t i = 0; i < got; i++) pcm.push_back(buf[i]);
        }
        size_t k = pcm.size() > 1000 ? pcm.size() - 1000 : 0;
        bool silent = true;
        for (size_t i = k; i < pcm.size(); i++) if (pcm[i] > 40 || pcm[i] < -40) { silent = false; break; }
        quiet = silent ? quiet + 1 : 0;
    }
    while (!pcm.empty() && pcm.back() > -40 && pcm.back() < 40) pcm.pop_back();       // trim trailing silence

    FILE *out = std::fopen(argv[2], "wb");
    if (!out) return 5;
    uint32_t rate = GM_AUDIO_RATE, byte_rate = rate * 2, bytes = (uint32_t)pcm.size() * 2, riff = 36 + bytes, fmt = 16;
    uint16_t one = 1, ch = 1, align = 2, bits = 16;
    std::fwrite("RIFF", 1, 4, out); std::fwrite(&riff, 4, 1, out); std::fwrite("WAVEfmt ", 1, 8, out);
    std::fwrite(&fmt, 4, 1, out); std::fwrite(&one, 2, 1, out); std::fwrite(&ch, 2, 1, out);
    std::fwrite(&rate, 4, 1, out); std::fwrite(&byte_rate, 4, 1, out); std::fwrite(&align, 2, 1, out);
    std::fwrite(&bits, 2, 1, out); std::fwrite("data", 1, 4, out); std::fwrite(&bytes, 4, 1, out);
    std::fwrite(pcm.data(), 2, pcm.size(), out);
    std::fclose(out);
    std::printf("%.3f\n", (double)pcm.size() / GM_AUDIO_RATE);
    return 0;
}

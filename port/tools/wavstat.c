/* Prints a timeline of a 16-bit mono WAV (as written by gmplay with GM_WAV=...): for every window the peak
 * level, RMS, and the strongest frequency (FFT), so audio output can be checked without listening.
 *
 *   wavstat file.wav [window_seconds]
 *   wavstat file.wav -s          one line: duration, peak and RMS (dBFS), number of clipped samples
 */
#define _USE_MATH_DEFINES 1
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N 8192

static void fft(double *re, double *im)
{
    for (int i = 1, j = 0; i < N; i++) {
        int bit = N >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) { double t = re[i]; re[i] = re[j]; re[j] = t; t = im[i]; im[i] = im[j]; im[j] = t; }
    }
    for (int len = 2; len <= N; len <<= 1) {
        double ang = -2 * M_PI / len, wr = cos(ang), wi = sin(ang);
        for (int i = 0; i < N; i += len) {
            double cr = 1, ci = 0;
            for (int k = 0; k < len / 2; k++) {
                int a = i + k, b = i + k + len / 2;
                double xr = re[b] * cr - im[b] * ci, xi = re[b] * ci + im[b] * cr;
                re[b] = re[a] - xr; im[b] = im[a] - xi;
                re[a] += xr; im[a] += xi;
                double t = cr * wr - ci * wi; ci = cr * wi + ci * wr; cr = t;
            }
        }
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "usage: wavstat file.wav [window_seconds | -s]\n"); return 2; }
    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror(argv[1]); return 1; }
    uint8_t hdr[44];
    if (fread(hdr, 1, 44, f) != 44 || memcmp(hdr, "RIFF", 4)) { fprintf(stderr, "not a WAV\n"); return 1; }
    uint32_t rate = hdr[24] | (hdr[25] << 8) | (hdr[26] << 16) | ((uint32_t)hdr[27] << 24);
    fseek(f, 0, SEEK_END); long bytes = ftell(f) - 44; fseek(f, 44, SEEK_SET);
    long n = bytes / 2;
    int16_t *s = malloc(n * 2);
    if (fread(s, 2, n, f) != (size_t)n) return 1;
    fclose(f);

    if (argc > 2 && !strcmp(argv[2], "-s")) {            /* one line: duration, peak and RMS in dBFS, clipped samples */
        double sum = 0; int peak = 0; long clipped = 0;
        for (long i = 0; i < n; i++) { sum += (double)s[i] * s[i]; int a = abs(s[i]); if (a > peak) peak = a; if (a >= 32767) clipped++; }
        double rms = n ? sqrt(sum / n) : 0;
        printf("%s %.1f s peak %.1f dBFS rms %.1f dBFS clipped %ld\n", argv[1], (double)n / rate,
               peak ? 20 * log10(peak / 32768.0) : -99.0, rms > 0 ? 20 * log10(rms / 32768.0) : -99.0, clipped);
        return 0;
    }

    double win = argc > 2 ? atof(argv[2]) : 1.0;
    long step = (long)(win * rate);
    printf("%s: %.2f s at %u Hz, mono 16-bit\n", argv[1], (double)n / rate, rate);
    printf("%6s %7s %7s %9s\n", "t(s)", "peak", "rms", "main Hz");
    double total_rms = 0; int windows = 0, silent = 0, maxpeak = 0;
    for (long start = 0; start < n; start += step) {
        long end = start + step < n ? start + step : n;
        double sum = 0; int peak = 0;
        for (long i = start; i < end; i++) { sum += (double)s[i] * s[i]; int a = abs(s[i]); if (a > peak) peak = a; }
        double rms = sqrt(sum / (end - start));
        double hz = 0;
        if (rms > 50 && end - start >= N) {
            static double re[N], im[N];
            long mid = start + (end - start - N) / 2;
            for (int i = 0; i < N; i++) { re[i] = s[mid + i] * (0.5 - 0.5 * cos(2 * M_PI * i / N)); im[i] = 0; }
            fft(re, im);
            int best = 1; double bv = 0;
            for (int k = 2; k < N / 2; k++) { double m = re[k] * re[k] + im[k] * im[k]; if (m > bv) { bv = m; best = k; } }
            hz = (double)best * rate / N;
        }
        printf("%6.1f %7d %7.0f %9.1f\n", (double)start / rate, peak, rms, hz);
        total_rms += rms; windows++; if (rms < 50) silent++; if (peak > maxpeak) maxpeak = peak;
    }
    printf("windows: %d, silent (rms<50): %d, mean rms %.0f, max peak %d\n", windows, silent,
           windows ? total_rms / windows : 0, maxpeak);
    return 0;
}

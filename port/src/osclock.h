/* Monotonic clock and sleep that work without a window (raylib's GetTime() and WaitTime() need an initialised
 * window system). Implemented in dosfind.c, where <windows.h> may be included. */
#ifndef GM_OSCLOCK_H
#define GM_OSCLOCK_H
#ifdef __cplusplus
extern "C" {
#endif
double gm_os_time(void);              /* seconds since the first call */
void   gm_os_sleep_ms(unsigned ms);
void   gm_os_yield(void);             /* web build: let the browser run (no-op elsewhere) */
#ifdef __cplusplus
}
#endif
#endif

/* Per-user settings of the port (window, volume, gamepad), kept in a small text file:
 *   Windows  %APPDATA%\gmplay\gmplay.ini
 *   macOS    ~/Library/Application Support/gmplay/gmplay.ini
 *   others   $XDG_CONFIG_HOME/gmplay/gmplay.ini, or ~/.config/gmplay/gmplay.ini
 * GM_CONFIG_DIR=<dir> overrides the folder; GM_NO_SETTINGS=1 neither reads nor writes the file (the test scripts
 * set it so that a person's settings cannot change a test).
 */
#ifndef GM_SETTINGS_H
#define GM_SETTINGS_H
#ifdef __cplusplus
extern "C" {
#endif

enum { GM_SCALE_INT43, GM_SCALE_FIT43, GM_SCALE_INTSQ, GM_SCALE_FITSQ, GM_SCALE_COUNT };

typedef struct {
    int fullscreen;     /* 0 = window, 1 = borderless full screen */
    int scale_mode;     /* GM_SCALE_*: integer or fitted, 4:3 (as on a monitor of the time) or square pixels */
    int volume;         /* 0..100 */
    int mute;
    int gamepad;        /* 1 = the first gamepad plays (joystick in games, arrows/Enter/Esc in menus) */
} GmSettings;

extern GmSettings gm_settings;

void gm_settings_load(void);        /* defaults, then the file */
void gm_settings_save(void);
const char *gm_scale_name(int mode);

#ifdef __cplusplus
}
#endif
#endif

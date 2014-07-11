/* sbmidi function prototypes */

/* MIDI output functions */
int sbmidi_out_shortmsg(unsigned char, unsigned char, unsigned char);
int sbmidi_out_longmsg(char *, int);

/* MIDI input functions */
void sbmidi_start_input(void);
void sbmidi_stop_input(void);
unsigned long sbmidi_get_input(void);

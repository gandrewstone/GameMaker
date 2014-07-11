

     /*  Sound Blaster Card's MIDI code routine  */
     
extern "C"
  {
  int  sbfd_init(void) ;
  int  sbfd_instrument(char far*) ;
  int  sbfd_reset(void) ;
  int  sbfd_music_off(void) ;
  int  sbfd_note_on(char,char,char) ;
  int  sbfd_note_off(char,char,char) ;
  int  sbfd_setmode(char) ;
  void sbfd_program_change(char,char) ;
  }

extern  unsigned  near ct_music_status ;

extern "C"
  { 
  int sbfm_init(void) ;
  int sbfm_version(void) ;
  void sbfm_terminate(void);
  char far *sbfm_status_addx(char far *) ;
  void sbfm_instrument(char far *,char) ;
  void sbfm_sys_speed(int) ;
  void sbfm_song_speed(int) ;
  void sbfm_transpose(int) ;
  void sbfm_play_music(char far *) ;
  void sbfm_stop_music(void) ;
  void sbfm_reset(void) ;
  void sbfm_pause_music(void) ;
  void sbfm_resume_music(void) ;
  int  sbfm_read_status(void) ;
  }

   typedef struct
   {
	  char		 id[4] ;
	  int		 version ;
	  int		 inst_blk ;
	  int		 music_blk ;
	  int		 ticks_per_beat ;
	  int		 clock_ticks ;
	  int		 music_title ;
	  int		 composer_name ;
	  int		 music_remark ;
	  char		 channel_used[16] ;
	  int		 inst_num ;
	  int		 basic_tempo ;

   } CMFHDR ;




#ifndef   ct_io_addx
     extern unsigned	 near ct_io_addx ;
     extern unsigned	 near ct_int_num ;
#endif

#ifndef   NO_ERROR
     #define NO_ERROR		 0
#endif

     extern unsigned	 near ct_voice_status ;


     /*   creative voice file disk version driver    */

int  ctvd_init(int) ;
void ctvd_terminate(void) ;
void ctvd_speaker(int) ;
int  ctvd_output(int) ;
int  ctvd_input(int,int) ;
void ctvd_stop(void) ;
void ctvd_pause(void) ;
void ctvd_continue(void) ;
void ctvd_break_loop(int) ;
int  ctvd_drv_error(void) ;
int  ctvd_ext_error(void) ;



     /*   creative voice file memory version driver    */
 extern "C"
  { 
  int   ctvm_initial(void) ;
  void  ctvm_terminate(void) ;
  void CTVM_Speaker(int);
  int   ctvm_input(char far*,unsigned long,int) ;
  int   ctvm_output(char far*) ; 
  void  ctvm_pause(void) ;
  void  ctvm_continue(void) ;
  void  ctvm_stop(void) ;
  void  ctvm_break_loop(int) ;
  } 
     /*   creative voice file header	*/

     typedef   struct
     {
	  char		 id[20] ;
	  unsigned	 voice_offset ;
	  unsigned	 version ;
	  unsigned	 check_code ;
     } VOCHDR ;



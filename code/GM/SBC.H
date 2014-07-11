#ifndef   ct_io_addx
     extern   unsigned	 near	   ct_io_addx ;
     extern   unsigned	 near	   ct_int_num ;
#endif

unsigned ct_card_here(void) ;
unsigned ct_scan_card(void) ;

extern "C"
  {
  int sbc_check_card(void);
  int sbc_scan_int(void);
  }

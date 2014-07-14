;
; Copyright (C) 1990-1993 Mark Adler, Richard B. Wales, Jean-loup Gailly,
; Kai Uwe Rommel and Igor Mandrichenko.
; Permission is granted to any individual or institution to use, copy, or
; redistribute this software so long as all of the original files are included,
; that it is not sold for profit, and that this copyright notice is retained.
;
; match32.asm by Jean-loup Gailly.

; match32.asm, optimized version of longest_match() in deflate.c
; To be used only with 32 bit flat model. To simplify the code, the option
; -DDYN_ALLOC is not supported.
; This file is only optional. If you don't have an assembler, use the
; C version (add -DNO_ASM to CFLAGS in makefile and remove match.o
; from OBJI). If you have reduced WSIZE in zip.h, then change its value
; below.

        .386

        name    match

_BSS    segment  dword USE32 public 'BSS'
        extrn   _match_start  : dword
        extrn   _prev_length  : dword
        extrn   _good_match   : dword
        extrn   _strstart     : dword
        extrn   _max_chain_length : dword
        extrn   _prev         : word
        extrn   _window       : byte
_BSS    ends

DGROUP  group _BSS

_TEXT   segment dword USE32 public 'CODE'
        assume  cs: _TEXT, ds: DGROUP, ss: DGROUP

        public match_init_
        public longest_match_

        MIN_MATCH     equ 3
        MAX_MATCH     equ 258
        WSIZE         equ 32768         ; keep in sync with zip.h !
        MIN_LOOKAHEAD equ (MAX_MATCH+MIN_MATCH+1)
        MAX_DIST      equ (WSIZE-MIN_LOOKAHEAD)

; initialize or check the variables used in match.asm.

match_init_ proc near
        ret
match_init_ endp

; -----------------------------------------------------------------------
; Set match_start to the longest match starting at the given string and
; return its length. Matches shorter or equal to prev_length are discarded,
; in which case the result is equal to prev_length and match_start is
; garbage.
; IN assertions: cur_match is the head of the hash chain for the current
;   string (strstart) and its distance is <= MAX_DIST, and prev_length >= 1

; int longest_match(cur_match)

longest_match_ proc near

        cur_match    equ dword ptr [esp+20]
        ; return address                ; esp+16
        push    ebp                     ; esp+12
        push    edi                     ; esp+8
        push    esi                     ; esp+4
        push    ebx                     ; esp

;       match        equ esi
;       scan         equ edi
;       chain_length equ ebp
;       best_len     equ ebx
;       limit        equ edx

        mov     esi,cur_match
        mov     ebp,_max_chain_length   ; chain_length = max_chain_length
        mov     edi,_strstart
        mov     edx,edi
        sub     edx,MAX_DIST            ; limit = strstart-MAX_DIST
        jae     short limit_ok
        sub     edx,edx                 ; limit = NIL
limit_ok:
        add     edi,2+offset _window    ; edi = offset(window + strstart + 2)
        mov     ebx,_prev_length        ; best_len = prev_length
        mov     ax,[ebx+edi-3]          ; ax = scan[best_len-1..best_len]
        mov     cx,[edi-2]              ; cx = scan[0..1]
        cmp     ebx,_good_match         ; do we have a good match already?
        jb      do_scan
        shr     ebp,2                   ; chain_length >>= 2
        jmp     short do_scan

        align   4                       ; align destination of branch
long_loop:
; at this point, edi == scan+2, esi == cur_match
        mov     ax,[ebx+edi-3]          ; ax = scan[best_len-1..best_len]
        mov     cx,[edi-2]              ; cx = scan[0..1]
short_loop:
        dec     ebp                     ; --chain_length
        jz      the_end
; at this point, edi == scan+2, esi == cur_match,
; ax = scan[best_len-1..best_len] and cx = scan[0..1]
        and     esi,WSIZE-1
        mov     si,_prev[esi+esi]       ; cur_match = prev[cur_match]
                                        ; top word of esi is still 0
        cmp     esi,edx                 ; cur_match <= limit ?
        jbe     short the_end
do_scan:
        cmp     ax,word ptr _window[ebx+esi-1]   ; check match at best_len-1
        jne     short_loop
        cmp     cx,word ptr _window[esi]         ; check min_match_length match
        jne     short_loop

        lea     esi,_window[esi+2]      ; si = match
        mov     eax,edi                 ; ax = scan+2
        mov     ecx,(MAX_MATCH-2)/2     ; scan for at most MAX_MATCH bytes
        repe    cmpsw                   ; loop until mismatch
        je      maxmatch                ; match of length MAX_MATCH?
mismatch:
        mov     cl,[edi-2]              ; mismatch on first or second byte?
        sub     cl,[esi-2]              ; cl = 0 if first bytes equal
        xchg    eax,edi                 ; edi = scan+2, eax = end of scan
        sub     eax,edi                 ; eax = len
        sub     esi,eax                 ; esi = cur_match + 2 + offset(window)
        sub     esi,2+offset _window    ; esi = cur_match
        sub     cl,1                    ; set carry if cl == 0 (can't use DEC)
        adc     eax,0                   ; eax = carry ? len+1 : len
        cmp     eax,ebx                 ; len > best_len ?
        jle     long_loop
        mov     _match_start,esi        ; match_start = cur_match
        mov     ebx,eax                 ; ebx = best_len = len
        cmp     eax,MAX_MATCH           ; len >= MAX_MATCH ?
        jl      long_loop
the_end:
        mov     eax,ebx                 ; result = eax = best_len
        pop     ebx
        pop     esi
        pop     edi
        pop     ebp
        ret
maxmatch:                               ; come here if maximum match
        cmpsb                           ; increment esi and edi
        jmp     mismatch                ; force match_length = MAX_LENGTH

longest_match_ endp

_TEXT   ends
end

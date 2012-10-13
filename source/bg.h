#ifndef __BG_H__
#define __BG_H__
/***********************************************
   Name: C code export for GBA
Version: 1.2
 Author: Sean Reid
  Email: email@seanreid.ca
Description:

This is the export file for GBA projects written in C.
This version exports 2 files:
    <filename>
    extern_<filename>
There are two files because one defines the graphics
as "extern", and the other contains the full arrays.


***********************************************/
#ifndef TONC_MAIN
#include <tonc.h>
#endif

extern const u16 PAL_BG[];
extern const u8 TS_BG[];
extern const u16 MAP_BG3[];
extern const u16 MAP_BG2[];
extern const u16 MAP_BG1[];
#endif

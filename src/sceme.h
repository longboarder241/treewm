/* treewm - an X11 window manager.
 * Copyright (c) 2001-2002 Thomas Jäger <TheHunter2000 at web dot de>
 * This code is released under the terms of the GNU GPL.
 * See the included file LICENSE for details.
 */

#include "resmanager.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifndef SCEME_H
#define SCEME_H


#define C_FG     0 // foreground color
#define C_BG     1 // background color
#define C_BD     2 // Border color
#define C_TBG    3 // title background
#define C_HTBG   4 // highlighted title background
#define C_HHTBG  5 // double-highlighted title background
#define C_IFG    6 // icon foreground
#define C_IBG    7 // icon background
#define C_HIBG    8 // highlighted icon background
#define C_IBD    9 // icon border
#define C_MFG   10 // menu foreground
#define C_MBG   11 // menu background
#define C_HMBG  12 // highlighted menu background
#define C_MBD   13 // menu border
#define C_DFG   14
#define C_DBG   15
#define C_DBD   16
#define C_NUM   17

#define FO_STD 0
#define FO_ICON 1
#define FO_MENU 2
#define FO_DIALOG 3
#define FO_NUM 4

#define CU_STD 0
#define CU_MOVE 1
#define CU_RESIZE 2
#define CU_NEWTARGET 3
#define CU_NONE 4
#define CU_NUM 5

#define BU_10 0
#define BU_11 1
#define BU_12 2
#define BU_13 3
#define BU_20 4
#define BU_21 5
#define BU_22 6
#define BU_23 7
#define BU_NUM 8

class Sceme : public Info {
  public:
    Sceme(Section *);
    virtual ~Sceme();	
    virtual void Init();
    XColor colors[C_NUM];
    XFontSet fonts[FO_NUM];
    Cursor cursors[CU_NUM];;
    RPixmap *button[BU_NUM];
    GC invert_gc, string_gc, border_gc, title_gc[3], icon_gc, menustring_gc,
    menu_gc,hmenu_gc,menubd_gc,dialogstring_gc;
    RPixmap *iconpm,*bgpm;
    int MinIX, MinIY, GridX, GridY, ISLeft, ISRight, ISTop, ISBottom;
    int BW;
    unsigned int DCTime;
    bool ShowKeys;

    Section *s;
};


#endif

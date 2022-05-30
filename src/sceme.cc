/* treewm - an X11 window manager.
 * Copyright (c) 2001-2003 Thomas Jäger <TheHunter2000 at web dot de>
 * This code is released under the terms of the GNU GPL.
 * See the included file LICENSE for details.
 *
 * changed<07.02.2003> by Rudolf Polzer: fallback to font "fixed"
 */

#include "global.h"
#include "sceme.h"
#include <X11/cursorfont.h>

// #define ICONPATH "~/.MyFavoriteIcon"

#define STDFONT "-*-lucida-medium-r-*-sans-10-*-*-*-*-*-*-*"
#define CMAP DefaultColormap(dpy, screen)

XFontSet LoadQueryFontSet(const char *fontset_name) {
  XFontSet fontset;
  int  missing_charset_count;
  char **missing_charset_list;
  char *def_string;

  fontset = XCreateFontSet(dpy, fontset_name,
                          &missing_charset_list, &missing_charset_count,
                          &def_string);
  if (missing_charset_count) {
    fprintf(stderr, "Missing charsets in FontSet(%s) creation.\n", fontset_name);
    XFreeStringList(missing_charset_list);

    fontset_name = "fixed";
    fontset = XCreateFontSet(dpy, fontset_name,
                            &missing_charset_list, &missing_charset_count,
                            &def_string);
    if (missing_charset_count) {
      fprintf(stderr, "Missing charsets in FontSet(%s) creation.\n", fontset_name);
      XFreeStringList(missing_charset_list);
      exit(1);
    }
  }
  return fontset;
}





Sceme::Sceme(Section *section) {
  s = section;
}

void Sceme::Init() {
  char *v; // value
  int i;

  XColor dummyc;
  v = s ? s->GetEntry("FGColor") : 0;
  XAllocNamedColor(dpy, CMAP, v ? v : "yellow", &colors[C_FG], &dummyc);
  v = s ? s->GetEntry("BGColor") : 0;
  XAllocNamedColor(dpy, CMAP, v ? v : "black", &colors[C_BG], &dummyc);
  v = s ? s->GetEntry("BDColor") : 0;
  XAllocNamedColor(dpy, CMAP, v ? v : "rgb:5/5/5", &colors[C_BD], &dummyc);
  v = s ? s->GetEntry("TitleBGColor") : 0;
  XAllocNamedColor(dpy, CMAP, v ? v : "rgb:0/0/5", &colors[C_TBG], &dummyc);
  v = s ? s->GetEntry("HTitleBGColor") : 0;
  XAllocNamedColor(dpy, CMAP, v ? v : "rgb:0/0/8", &colors[C_HTBG], &dummyc);
  v = s ? s->GetEntry("HHTitleBGColor") : 0;
  XAllocNamedColor(dpy, CMAP, v ? v : "rgb:0/0/B", &colors[C_HHTBG], &dummyc);
  v = s ? s->GetEntry("IconFGColor") : 0;
  XAllocNamedColor(dpy, CMAP, v ? v : "white", &colors[C_IFG], &dummyc);
  v = s ? s->GetEntry("IconBGColor") : 0;
  XAllocNamedColor(dpy, CMAP, v ? v : "grey15", &colors[C_IBG], &dummyc);
  v = s ? s->GetEntry("HIconBGColor") : 0;
  XAllocNamedColor(dpy, CMAP, v ? v : "grey45", &colors[C_HIBG], &dummyc);
  v = s ? s->GetEntry("IconBDColor") : 0;
  XAllocNamedColor(dpy, CMAP, v ? v : "grey", &colors[C_IBD], &dummyc);
  v = s ? s->GetEntry("MenuFGColor") : 0;
  XAllocNamedColor(dpy, CMAP, v ? v : "black", &colors[C_MFG], &dummyc);
  v = s ? s->GetEntry("MenuBGColor") : 0;
  XAllocNamedColor(dpy, CMAP, v ? v : "grey", &colors[C_MBG], &dummyc);
  v = s ? s->GetEntry("HMenuBGColor") : 0;
  XAllocNamedColor(dpy, CMAP, v ? v : "yellow", &colors[C_HMBG], &dummyc);
  v = s ? s->GetEntry("MenuBDColor") : 0;
  XAllocNamedColor(dpy, CMAP, v ? v : "black", &colors[C_MBD], &dummyc);
  v = s ? s->GetEntry("DialogFGColor") : 0;
  XAllocNamedColor(dpy, CMAP, v ? v : "yellow", &colors[C_DFG], &dummyc);
  v = s ? s->GetEntry("DialogBGColor") : 0;
  XAllocNamedColor(dpy, CMAP, v ? v : "black", &colors[C_DBG], &dummyc);
  v = s ? s->GetEntry("DialogBDColor") : 0;
  XAllocNamedColor(dpy, CMAP, v ? v : "rgb:5/5/5", &colors[C_DBD], &dummyc);
  v = s ? s->GetEntry("Font") : 0;
  fonts[FO_STD] = LoadQueryFontSet(v ? v : STDFONT);
  v = s ? s->GetEntry("IconFont") : 0;
  fonts[FO_ICON] = v ? LoadQueryFontSet(v)
                     : fonts[FO_STD]; // :-)
  v = s ? s->GetEntry("MenuFont") : 0;
  fonts[FO_MENU] = v ? LoadQueryFontSet(v)
                     : fonts[FO_STD]; // :-)
  v = s ? s->GetEntry("DialogFont") : 0;
  fonts[FO_DIALOG] = v ? LoadQueryFontSet(v)
                     : fonts[FO_STD]; // :-)
  for (int i=0;i!=FO_NUM;++i) {
    if (!fonts[i])
      fonts[i] = LoadQueryFontSet(STDFONT);
    if (!fonts[i]) {
      fprintf(stderr,"No font found\n");
      exit(1);
    }
  }
  cursors[CU_STD] = XCreateFontCursor(dpy, XC_left_ptr);
  cursors[CU_MOVE] = XCreateFontCursor(dpy, XC_fleur);
  cursors[CU_RESIZE] = XCreateFontCursor(dpy, XC_plus);
  cursors[CU_NEWTARGET] = XCreateFontCursor(dpy, XC_diamond_cross);

  { // stolen from xawtv
    XColor black, dummy;
    Pixmap bm_no;
    const char bm_no_data[] = { 0,0,0,0, 0,0,0,0 };
    bm_no = XCreateBitmapFromData(dpy, root, bm_no_data, 8,8);
    XAllocNamedColor(dpy,CMAP,"black",&black,&dummy);
    cursors[CU_NONE] = XCreatePixmapCursor(dpy, bm_no, bm_no, &black, &black, 0, 0);
		XFreePixmap(dpy,bm_no);
  }

  v = s ? s->GetEntry("defaulticon") : 0;
  if (v) {
    iconpm = rman->GetPixmap(v);
  } else {
#ifdef ICONPATH
    iconpm = rman->GetPixmap(ICONPATH);
#else
    iconpm = 0;
#endif
  }
  if (s && s->GetIntEntry("borderwidth",i)) {
    BW = i;
  } else BW = 1;

  v = s ? s->GetEntry("bgpixmap") : 0;
  if (v) {
    bgpm = rman->GetPixmap(v);
  } else bgpm = 0;


  v = s ? s->GetEntry("button10") : 0;
  button[BU_10] = rman->GetPixmap(v ? v : "none1.xpm");
  v = s ? s->GetEntry("button11") : 0;
  button[BU_11] = rman->GetPixmap(v ? v : "max2.xpm");
  v = s ? s->GetEntry("button12") : 0;
  button[BU_12] = rman->GetPixmap(v ? v : "max1.xpm");
  v = s ? s->GetEntry("button13") : 0;
  button[BU_13] = rman->GetPixmap(v ? v : "resize.xpm");
  v = s ? s->GetEntry("button20") : 0;
  button[BU_20] = rman->GetPixmap(v ? v : "none2.xpm");
  v = s ? s->GetEntry("button21") : 0;
  button[BU_21] = rman->GetPixmap(v ? v : "close.xpm");
  v = s ? s->GetEntry("button22") : 0;
  button[BU_22] = rman->GetPixmap(v ? v : "icon.xpm");
  v = s ? s->GetEntry("button23") : 0;
  button[BU_23] = rman->GetPixmap(v ? v : "move.xpm");
  for (int i=0;i!=BU_NUM;++i)
    if (button[i])
      button[i]->GetPixmap();


  if (!s || !s->GetIntEntry("ShowKeys",i) || i)
    ShowKeys = true; else
    ShowKeys = false;


  if (!s || !s->GetIntEntry("MinIconX",MinIX))
    MinIX = 100;
  if (!s || !s->GetIntEntry("MinIconY",MinIY))
    MinIY = 50;
  if (!s || !s->GetIntEntry("GridX",GridX))
    GridX = 80;
  if (!s || !s->GetIntEntry("GridY",GridY))
    GridY = 70;
  if (!s || !s->GetIntEntry("IconSpaceLeft",ISLeft))
    ISLeft = 35;
  if (!s || !s->GetIntEntry("IconSpaceRight",ISRight))
    ISRight = 35;
  if (!s || !s->GetIntEntry("IconSpaceTop",ISTop))
    ISTop = 40;
  if (!s || !s->GetIntEntry("IconSpaceBottom",ISBottom))
    ISBottom = 25;
  if (!s || !s->GetIntEntry("DoubleClickTime",i))
    i = 250;
  DCTime = i;

  XGCValues gv;
// delete some gcs!!
  gv.graphics_exposures = false;
  gv.function = GXcopy;

  gv.foreground = colors[C_IFG].pixel;
//  gv.font = fonts[FO_ICON]->fid;
  icon_gc = XCreateGC(dpy, root, GCFunction|GCForeground|GCGraphicsExposures, &gv);

  gv.foreground = colors[C_DFG].pixel;
//  gv.font = fonts[FO_DIALOG]->fid;
  dialogstring_gc = XCreateGC(dpy, root, GCFunction|GCForeground|GCGraphicsExposures, &gv);

  gv.foreground = colors[C_MFG].pixel;
//  gv.font = fonts[FO_MENU]->fid;
  menustring_gc = XCreateGC(dpy, root, GCFunction|GCForeground|GCGraphicsExposures, &gv);

  gv.foreground = colors[C_FG].pixel;
//  gv.font = fonts[FO_STD]->fid;
  string_gc = XCreateGC(dpy, root, GCFunction|GCForeground|GCGraphicsExposures, &gv);

  gv.foreground = colors[C_MBG].pixel;
  menu_gc = XCreateGC(dpy, root, GCFunction|GCForeground|GCGraphicsExposures, &gv);

  gv.foreground = colors[C_HMBG].pixel;
  hmenu_gc = XCreateGC(dpy, root, GCFunction|GCForeground|GCGraphicsExposures, &gv);

  gv.foreground = colors[C_MBD].pixel;
  menubd_gc = XCreateGC(dpy, root, GCFunction|GCForeground|GCGraphicsExposures, &gv);

  gv.foreground = colors[C_BD].pixel;
  gv.line_width = 1;
  border_gc = XCreateGC(dpy, root, GCFunction|GCForeground|GCGraphicsExposures|GCLineWidth, &gv);

  gv.foreground = colors[C_TBG].pixel;
  title_gc[0] = XCreateGC(dpy, root, GCFunction|GCForeground|GCGraphicsExposures, &gv);

  gv.foreground = colors[C_HTBG].pixel;
  title_gc[1] = XCreateGC(dpy, root, GCFunction|GCForeground|GCGraphicsExposures, &gv);

  gv.foreground = colors[C_HHTBG].pixel;
  title_gc[2] = XCreateGC(dpy, root, GCFunction|GCForeground|GCGraphicsExposures, &gv);

  gv.function = GXinvert;
  gv.subwindow_mode = IncludeInferiors;
  invert_gc = XCreateGC(dpy, root, GCFunction|GCSubwindowMode|GCGraphicsExposures|GCLineWidth, &gv);


  s = 0;

}


Sceme::~Sceme() {
  if (fonts[FO_STD] == fonts[FO_ICON])
    fonts[FO_ICON] = 0;
  if (fonts[FO_STD] == fonts[FO_MENU])
    fonts[FO_MENU] = 0;
  if (fonts[FO_STD] == fonts[FO_DIALOG])
    fonts[FO_DIALOG] = 0;
  for (int i=0;i!=FO_NUM;++i)
    if (fonts[i])
      XFreeFontSet(dpy, fonts[i]);
  for (int i=0;i!=CU_NUM;++i)
    if (cursors[i])
      XFreeCursor(dpy, cursors[i]);
  XFreeGC(dpy, invert_gc);
  XFreeGC(dpy, border_gc);
  XFreeGC(dpy, string_gc);
  XFreeGC(dpy, icon_gc);
  XFreeGC(dpy, title_gc[0]);
  XFreeGC(dpy, title_gc[2]);
  XFreeGC(dpy, title_gc[1]);
  XFreeGC(dpy, menu_gc);
  XFreeGC(dpy, menubd_gc);
  XFreeGC(dpy, menustring_gc);
  XFreeGC(dpy, hmenu_gc);
  XFreeGC(dpy, dialogstring_gc);
  for (int i=0;i!=BU_NUM;++i)
    if (button[i])
      button[i]->FreePixmap();

}

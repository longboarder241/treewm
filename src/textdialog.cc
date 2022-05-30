/* treewm - an X11 window manager.
 * Copyright (c) 2001-2002 Thomas Jäger <TheHunter2000 at web dot de>
 * This code is released under the terms of the GNU GPL.
 * See the included file LICENSE for details.
 */

#include "textdialog.h"
#include "sceme.h"
#include "uehandler.h"
#include <X11/keysym.h>

void TextDialog::AddHistory() {
  if (!length)
    return;
  hist[histpos] = strdup(line);
  ++histpos;
  histpos %= HISTSIZE;
  histref = histpos;
  if (hist[histpos])
    free(hist[histpos]);
  hist[histpos] = 0;
}

void TextDialog::ChangeHist(int a) {
  if (!hist[(histref + a) % HISTSIZE])
    return;
  histref = (histref + a) % HISTSIZE;

  strcpy(line,hist[histref]);
  length = strlen(line);
  cur = length;
  ReDraw();
}

char *TextDialog::hist[HISTSIZE];
int TextDialog::histpos;


char *TextDialog::Key(XKeyEvent &ev) {
  char buf[2];
  KeySym ks = XLookupKeysym(&ev, 0);
  if (ev.state & Mod1Mask) {
    switch (ks) {
      case XK_space:
        return strdup(line);
      case XK_Return:
        return strdup(line);
      default:
        return 0;
    }
  }

  switch (ks) {
    case XK_Return:
      if (length == 0)
        return strdup(line);
      AddHistory();
      UEH->ExecCommand(0,0,line,true);
      cur = 0;
      length = 0;
      line[0] = '\0';
      ReDraw();
      return 0;
    case XK_Escape:
      return strdup("");
    case XK_Home:
      cur = 0;
      ReDraw();
      return 0;
    case XK_End:
      cur = length;
      ReDraw();
      return 0;
    case XK_Left:
      if (cur) {
        --cur;
        ReDraw();
      }
      return 0;
    case XK_Right:
      if (cur != length) {
        ++cur;
        ReDraw();
      }
      return 0;
    case XK_BackSpace:
      if (!cur)
        return 0;
      --cur;
    case XK_Delete:
      if (cur == length)
        return 0;
      for (int i=cur;i!=length;i++)
        line[i]=line[i+1];
      line[--length] = '\0';
      ReDraw();
      return 0;
    case XK_Up:
    case XK_Page_Up:
      ChangeHist(HISTSIZE - 1);
      return 0;
    case XK_Down:
    case XK_Page_Down:
      ChangeHist(1);
      return 0;
    case XK_Tab:
      Complete();
      return 0;
  }

  // XmbLookupString ??
  if (1 != XLookupString(&ev,buf,1,0,&cstatus)) {
#ifdef DEBUG
    err("%i",cstatus.chars_matched);
#endif
    return 0;
  }


  if (length == 0) {
    buf[1] = '\0';
    if (UEH->ExecCommand(0,0,buf,false))
      return 0;
  }
  if (length == MAXLENGTH-1)
    return 0;
  for (int i=length;i>=cur;i--)
    line[i+1]=line[i];
  line[cur++] = buf[0];
  ++length;
  ReDraw();
  return 0;
}


TextDialog::TextDialog(Sceme *SC) {
  Sc = SC;
  histref = histpos;
  XSetWindowAttributes pattr;

  cstatus.compose_ptr = 0;
  cstatus.chars_matched = 0;

  XFontSetExtents *e = XExtentsOfFontSet(Sc->fonts[FO_DIALOG]);
  height = e->max_logical_extent.height*4/5 + e->max_logical_extent.height/5 + 2*DIALOGSPACE;

  int sw = DisplayWidth(dpy,screen);
  int sh = DisplayHeight(dpy,screen);
  int space = sw/64;
  pattr.override_redirect = True;
  pattr.background_pixel = Sc->colors[C_DBG].pixel;
  pattr.border_pixel = Sc->colors[C_DBD].pixel;
  pattr.event_mask = ChildMask|ButtonMask|ExposureMask|EnterWindowMask|KeyPressMask;
  window = XCreateWindow(dpy, root, space, sh-height-space, sw-2*space, height, 1,
      DefaultDepth(dpy, screen), CopyFromParent, DefaultVisual(dpy, screen),
      CWOverrideRedirect|CWBackPixel|CWBorderPixel|CWEventMask, &pattr);
  XMapWindow(dpy,window);
  XGrabKeyboard(dpy,window,false,GrabModeAsync,GrabModeAsync,CurrentTime);

  cur = 0;
  length = 0;
  line[0] = '\0';
}

void TextDialog::ReDraw() {
  XClearWindow(dpy,window);
  XFontSetExtents *e = XExtentsOfFontSet(Sc->fonts[FO_DIALOG]);
  XmbDrawString(dpy, window, Sc->fonts[FO_DIALOG], Sc->dialogstring_gc, DIALOGSPACE,
              DIALOGSPACE + e->max_logical_extent.height*4/5, line, length);
  int curpos = DIALOGSPACE + XmbTextEscapement(Sc->fonts[FO_DIALOG],line,cur);
  XDrawLine(dpy,window,Sc->invert_gc,curpos,DIALOGSPACE,curpos,height - DIALOGSPACE);
}

void TextDialog::SetCursor(int x) {
  x -= DIALOGSPACE;
  if (x < 0) {
    cur = 0;
    ReDraw();
    return;
  }
  if (x > XmbTextEscapement(Sc->fonts[FO_DIALOG],line,length)) {
    cur = length;
    ReDraw();
    return;
  }
  int i1=0,i2=length;
  while (i2 != i1+1) {
    int i = (i1 + i2) / 2;
    if (x > XmbTextEscapement(Sc->fonts[FO_DIALOG],line,i))
      i1 = i; else
      i2 = i;
  }
  if (XmbTextEscapement(Sc->fonts[FO_DIALOG],line,i1) +
        XmbTextEscapement(Sc->fonts[FO_DIALOG],line,i2) <= 2*x)
    cur = i2; else
    cur = i1;
  ReDraw();
}

void TextDialog::Paste() {
  char *buffer,*tmp;
  int rest;
  int nbytes;
  if ((buffer = XFetchBytes(dpy, &nbytes)) != NULL) {
    tmp = strdup(line+cur);
    rest = length - cur;
    strncpy(line+cur,buffer,MAXLENGTH - cur - 1);
    line[MAXLENGTH-1] = '\0';
    length = strlen(line);
    cur = length;
    if (length + rest > MAXLENGTH-1)
      cur = MAXLENGTH - 1 - rest;
    strcpy(line+cur,tmp);
    length = strlen(line);
    free(tmp);
  }
  XFree(buffer);
  ReDraw();
}

void TextDialog::SetLine(char *n) {
  strncpy(line, n, MAXLENGTH - 1);
  line[MAXLENGTH-1] = '\0';
  length = strlen(line);
  cur = length;
  ReDraw();
}

void TextDialog::Complete() {
// !!!
}


TextDialog::~TextDialog() {
  AddHistory();
  XUngrabKeyboard(dpy,CurrentTime);
  XUnmapWindow(dpy,window);
  XDestroyWindow(dpy,window);
}

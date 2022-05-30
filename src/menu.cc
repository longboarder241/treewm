/* treewm - an X11 window manager.
 * Copyright (c) 2001-2003 Thomas Jäger <TheHunter2000 at web dot de>
 * This code is released under the terms of the GNU GPL.
 * See the included file LICENSE for details.
 */

#include "menu.h"
#include "sceme.h"
#include "uehandler.h"
#include <X11/keysym.h>

inline int min(int x,int y) {
  return x < y ? x : y;
}

Menu::Menu(Menu *Parent, const MenuItem* Items,int N,Client *c, char *CMD,
    Sceme *sceme,int X,int Y, bool center) {
  parent = Parent;
  cmd = CMD;
  client = c;
  Sc = sceme;
  window = 0;
  int w,width2 = 0;
  width = 0;
  n = N;
  items = Items;
  buf = 0;
  ignore = 0;
  XFontSetExtents *e = XExtentsOfFontSet(Sc->fonts[FO_MENU]);
  EHeight = e->max_logical_extent.height*4/5 + e->max_logical_extent.height/5 + 2*MENUSPACE;
  height = n*EHeight - 1;
  for (int i=0;i!=n;++i) {
    w = XmbTextEscapement(Sc->fonts[FO_MENU],items[i].text,strlen(items[i].text));
    w += MENUINDENT * (items[i].flags/MF_INDENT);
    if (w>width)
      width = w;
    if (!(Sc->ShowKeys)/* || items[i].key[0] == ':'*/)
      continue;
    w = XmbTextEscapement(Sc->fonts[FO_MENU],items[i].key,strlen(items[i].key));
    if (w>width2)
      width2 = w;
  }
  width += width2 + 4*MENUSPACE;
  sel = 0;
  x = min(X - (center ? width/2 : 0), DisplayWidth(dpy, screen) - width - 2);
  if (x < 0)
    x = 0;
  y  = min(Y, DisplayHeight(dpy, screen) - height - 2);
  if (y < 0)
    y = 0;
  submenu = 0;
}


Menu::~Menu() {
  if (submenu)
    delete submenu;
}


void Menu::Init() {
  XSetWindowAttributes pattr;

  pattr.override_redirect = True;
  pattr.background_pixel = Sc->colors[C_MBG].pixel;
  pattr.border_pixel = Sc->colors[C_MBD].pixel;
  pattr.event_mask = ChildMask|MouseMask|ExposureMask|EnterWindowMask|KeyPressMask;
  window = XCreateWindow(dpy, root, x, y, width, height, 1,
      DefaultDepth(dpy, screen), CopyFromParent, DefaultVisual(dpy, screen),
      CWOverrideRedirect|CWBackPixel|CWBorderPixel|CWEventMask, &pattr);
  XMapWindow(dpy,window);
  if (!parent) {
    XGrabPointer(dpy,window,false,MouseMask,GrabModeAsync,GrabModeAsync,
	None,None,CurrentTime);
    if (!UEH->dialog)
      XGrabKeyboard(dpy,window,false,GrabModeAsync,GrabModeAsync,CurrentTime);
  }
}

void Menu::DrawItem(int i) {
  XFontSetExtents *e = XExtentsOfFontSet(Sc->fonts[FO_MENU]);
  XmbDrawString(dpy, window, Sc->fonts[FO_MENU], Sc->menustring_gc,
                MENUSPACE + MENUINDENT * (items[i].flags/MF_INDENT),
                i*EHeight + MENUSPACE + e->max_logical_extent.height*4/5,
                items[i].text, strlen(items[i].text));
  if (!(Sc->ShowKeys) /* || items[i].key[0] == ':'*/)
    return;
  int w = XmbTextEscapement(Sc->fonts[FO_MENU],items[i].key,strlen(items[i].key));
  XmbDrawString(dpy, window, Sc->fonts[FO_MENU], Sc->menustring_gc, width -w-MENUSPACE,
                i*EHeight + MENUSPACE +  e->max_logical_extent.height*4/5,
                items[i].key, strlen(items[i].key));

}

void Menu::ReDraw() {
  XClearWindow(dpy,window);
  for (int i=0;i!=n;++i) {
    if (i)
      XDrawLine(dpy, window, Sc->menubd_gc, 0, i*EHeight-1, width, i*EHeight-1);
      if (i+1 == sel)
        XFillRectangle(dpy,window,Sc->hmenu_gc,0,i*EHeight,width,EHeight-1);
      DrawItem(i);
  }
}

char *Menu::GetCmd() {
  if (!sel)
    return 0;
  char *key = items[sel-1].key;
  if (!cmd)
    return key;
  if (!key)
    return cmd;
  if (buf)
    delete [] buf;
  buf = new char[strlen(key) + strlen(cmd) + 1];
  strcpy(buf,cmd);
  strcat(buf,key);
  return buf;
}

void Menu::Mouse(int X,int Y) {
  if (submenu) {
    submenu->Mouse(X,Y);
    if (submenu->sel)
      return;
  }
  int oldsel = sel;
  if (x <= X && X <= x+width && y <= Y && Y <= y + height)
    sel = (Y-y)/EHeight + 1; else
    sel = 0;
  if (sel > n)
    sel = n;
  if (sel == oldsel)
    return;
  if (oldsel) {
    XFillRectangle(dpy,window,Sc->menu_gc,0,(oldsel-1)*EHeight,width,EHeight-1);
    DrawItem(oldsel - 1);
  }

  if (sel) {
    XFillRectangle(dpy,window,Sc->hmenu_gc,0,(sel-1)*EHeight,width,EHeight-1);
    DrawItem(sel - 1);
  }
  if (submenu) {
    if (sel == selsubmenu)
      return;
    submenu->Remove(0); // must return true ??
    delete submenu;
    submenu = 0;
  }
  if (!sel)
    return;
  SubMenuInfo *si = items[sel-1].submenu;
  if (si) {
    submenu = new Menu(this, si->menu, si->num, client, GetCmd(), Sc,
	x + width + Sc->BW, y + (sel-1)*EHeight, false);
    submenu->Init();
    selsubmenu = sel;
  }
}


bool Menu::Key(XKeyEvent &e) {
  if (submenu && !sel) {
    if (submenu->Key(e))
      return true;
    if (submenu->sel)
      return false;
    /* selsubmenu must be != 0 */
    XWarpPointer(dpy,None,root,0,0,0,0,x+width/2,y+EHeight*(selsubmenu-1)+EHeight/2);
    sel = selsubmenu;
    submenu->sel = 0;
    ReDraw();
    submenu->ReDraw();
    return true;
  }

  if (e.keycode == XKeysymToKeycode(dpy,XK_Up)) {
    if (sel > 1)
      XWarpPointer(dpy,None,root,0,0,0,0,x+width/2,y+EHeight*(sel-2)+EHeight/2);
    return true;
  }
  if (e.keycode == XKeysymToKeycode(dpy,XK_Down)) {
    if (sel < n)
      XWarpPointer(dpy,None,root,0,0,0,0,x+width/2,y+EHeight*sel+EHeight/2);
    return true;
  }
  if (e.keycode == XKeysymToKeycode(dpy,XK_Return) || 
      e.keycode == XKeysymToKeycode(dpy,XK_Right) ) {
    if (sel && submenu) {
      XWarpPointer(dpy,None,root,0,0,0,0,submenu->x+submenu->width/2,submenu->y+EHeight/2);
      sel = 0;
      submenu->sel = 1;
      ReDraw();
      submenu->ReDraw();
      return true;
    }
    return false;
  }
  if (e.keycode == XKeysymToKeycode(dpy,XK_Escape) ||
      e.keycode == XKeysymToKeycode(dpy,XK_Left) ) {
    sel = 0;
    return false;
  }
  return true;
}

bool Menu::Remove(int *ret) {
  if (ignore) {
    --ignore;
    return false;
  }
  if (submenu && !submenu->Remove(0))
    return false;
  char *Cmd = GetCmd();
  if (Cmd && Cmd[0] == '?' && Cmd[1] == '\0')
    return false;
  if (!parent) {
    XUngrabPointer(dpy,CurrentTime);
    if (!UEH->dialog)
      XUngrabKeyboard(dpy,CurrentTime);
  }
  XUnmapWindow(dpy,window);
  XDestroyWindow(dpy,window);
  if (sel) {
    Client *c = items[sel-1].client;
    if (!c)
      c = client;
    UEH->ExecCommand(c,0,Cmd,true);
  }
  if (buf)
    delete [] buf;
  if (ret)
    *ret = sel;
  return true;
}

/* treewm - an X11 window manager.
 * Copyright (c) 2001-2002 Thomas Jäger <TheHunter2000 at web dot de>
 * This code is released under the terms of the GNU GPL.
 * See the included file LICENSE for details.
 * 
 * changed<05.02.2003> by Rudolf Polzer: including namespace "std" (C++) 
 */

#ifndef GLOBAL_H
#define GLOBAL_H

#ifdef DEBUG2
#define DEBUG
#endif

using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif
#ifdef DEBUG
#include <stdarg.h>
#endif

#define TF_ISICON    (1L<<0)

//Class Declarations
class TWindow {
  public:
  unsigned long flags;
};
class Manager;
class Client;
class Desktop;
class Icon;
class Action;
class ClientInfo;
class RPixmap;
class ClientTree;
class Section;
class ResManager;
class Sceme;
class UEHandler;
class Menu;
class Info {
  public:
    virtual ~Info();
    virtual void Init() = 0;
};
class TextDialog;

#define ButtonMask (ButtonPressMask|ButtonReleaseMask|ButtonMotionMask)
#define ChildMask (SubstructureRedirectMask|SubstructureNotifyMask)
#define MouseMask (ButtonPressMask|ButtonReleaseMask|PointerMotionMask)


extern Display *dpy;
extern ClientTree *ct;
extern UEHandler *UEH;
extern Manager *man;
extern ResManager *rman;
extern int screen;
extern Window root;
extern Atom wm_state, wm_change_state, wm_protos, wm_delete, wm_cmapwins, wm_client_leader,
            mwm_hints;

int handle_xerror(Display *dpy, XErrorEvent *e);

#ifdef SHAPE
extern Bool shape;
extern int shape_event;
#endif

void lower(char * str);

#ifdef DEBUG
void ShowEvent(XEvent);
void err(const char *fmt, ...);
extern bool error;
#endif

#define MAXTITLE 150
#define MAXICON 30

#define UNDEF (-32768)


#endif

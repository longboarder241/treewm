/* treewm - an X11 window manager.
 * Copyright (c) 2001-2003 Thomas Jäger <TheHunter2000 at web dot de>
 * This code is released under the terms of the GNU GPL.
 * See the included file LICENSE for details.
 *
 * changed<05.02.2003> by Rudolf Polzer: sscanf format string bug (was
 *   [^\\0], now is a[^\n]
 */

#include "uehandler.h"
#include "clienttree.h"
#include "icon.h"
#include "sceme.h"
#include "action.h"
#include "menu.h"
#include "menuinfo.h"
#include "textdialog.h"
#include <X11/keysym.h>
#include <ctype.h>

MenuItem OptionMenu[] = {
  {"Shaded","shaded",0,0},
  {"AutoShade","autoshade",0,0},
  {"Fixed Size","fixedsize",0,0},
  {"Don't Close","noclose",0,0},
  {"Don't Minimize","nominimize",0,0},
  {"Sticky","sticky",0,0},
  {"Title","hastitle",0,0},
  {"TaskBar Entry","hastbentry",0,0},
  {"Grab Alt Clicks","grabaltclick",0,0},
  {"Pass First Click","passfirstclick",0,0},
  {"TitleBar Buttons","titlebarbuttons",0,0},
  {"Snap","snap",0,0},
  {"Always on Top","above",0,0},
  {"Below","below",0,0},
  {"Hide Mouse","hidemouse",0,0},
  {"Raise On Click","raiseonclick",0,0},
  {"Raise On Enter","raiseonenter",0,0},
  {"Focus On Click","focusonclick",0,0},
  {"Focus On Enter","focusonenter",0,0},
  {"Buttons","Buttons",0,0},
  {"TaskBar Buttons","taskbarbuttons",0,0},
  {"Simultaneous Input","grabkeyboard",0,0},
  {"Autoscroll","autoscroll",0,0},
  {"Tile","tile",0,0},
  {"Autoresize","autoresize",0,0}
};

const int ClientOptionMenuNum = 19;
const int DesktopOptionMenuNum = 25;

SubMenuInfo OptionMenuInfo = {(MenuItem *)OptionMenu, DesktopOptionMenuNum};

#define OMI &OptionMenuInfo

MenuItem ClientMenu[] = {
  {"Menu","?",0,0,0},
  {"Raise"," ",0,0,0},
  {"Lower","x",0,0,0},
  {"Create Desktop","D",0,0,0},
  {"Iconify","I",0,0,0},
  {"Shade","S",0,0,0},
  {"(Un)Stick","s",0,0,0},
  {"Maximize Full","M",0,0,0},
  {"Maximize Half","m",0,0,0},
  {"Maximize to Fullscreen","f",0,0,0},
  {"Move","c",0,0,0},
  {"Resize","C",0,0,0},
  {"Move Up","a",0,0,0},
  {"Move Down","A",0,0,0},
#ifdef SHAPE
  {"Add Hole","o",0,0,0},
  {"Delete Holes","O",0,0,0},
#endif
  {"Destroy!","Q",0,0,0},
  {"Close","q",0,0,0},
  {"Set Option",":set ",0,0,OMI},
  {"Unset Option",":unset ",0,0,OMI},
  {"Toggle Option",":toggle ",0,0,OMI},
  {"Remove Option",":remove ",0,0,OMI},
/* ----Desktop---- */  
  {"Toggle (only me)",":dtoggle ",0,0,OMI},
  {"Remove (only me)",":dremove ",0,0,OMI},
  {"Toggle (children)",":ctoggle ",0,0,OMI},
  {"Remove (children)",":cremove ",0,0,OMI},
  {"Show/Hide Icons","i",0,0,0},
  {"Release Windows","e",0,0,0},
  {"Rename","+=",0,0,0}
};

MenuItem IconMenu[] = {
  {"Menu","?",0,0,0},
  {"Execute"," ",0,0,0},
  {"Rename","+=",0,0,0},
  {"Remove","q",0,0,0}
};
const int IconMenuNum = 4;

#ifdef SHAPE
const int ClientMenuNum = 22;
const int DesktopMenuNum = 29;
#else
const int ClientMenuNum = 20;
const int DesktopMenuNum = 27;
#endif


const char *ModNames[] = {
	"Shift", "Lock", "Control", "Alt",
	"Mod1", "Mod2", "Mod3", "Mod4", "Mod5",
	"Button1", "Button2", "Button3", "Button4", "Button5", "AnyButton",
	0
};

unsigned int ModValues[] = {
	ShiftMask, LockMask, ControlMask, Mod1Mask,
	Mod1Mask, Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask,
	Button1, Button2, Button3, Button4, Button5, AnyButton
};


UEHandler *UEH;



void UEHandler::AddKey(KeySym key,unsigned int mod,char *cmd) {
  KeySym code = XKeysymToKeycode(dpy,key);
  ModList *ml = keys[code];
  if (!cmd) {
    ModList *prev = 0;
    for (;ml;ml = ml->next) {
      if (ml->mod == mod)
        break;
      prev = ml;
    }
    if (ml) {
      if (prev)
        prev->next = ml->next; else
        keys[code] = ml->next;
      delete ml;
      XUngrabKey(dpy,code,mod,root);
    }
    return;
  }
  ModList *ml2 = ml;
  for (;ml2;ml2 = ml2->next) {
    if (ml2->mod == mod)
      break;
  }
  if (!ml2) {
    ml2 = new ModList;
    ml2->next = ml;
    keys[code] = ml2;
  }
  XGrabKey(dpy,code,mod,root,false,GrabModeAsync,GrabModeAsync);
  ml2->mod = mod;
  ml2->cmd = cmd;
}



UEHandler::UEHandler() : keys() {
  UEH = this;
  Current = 0;
  CurrentIcon = 0;
  MoveWin   = 0;
  Seltype   = 0;
  downtime  = 0;
  NewTarget = 0;
  SelEntry  = 0;
  SelClient = 0;
  SelDesktop= 0;
  RefClient = 0;
  menu      = 0;
  menuitems = 0;
  preg      = 0;
  dialog    = 0;
  dreg = 0;
  for (int i = 'z'-'a';i+1;--i)
    reg[i] = 0;
  /* Add Keys {{{*/
  AddKey(XK_space, Mod1Mask, "+");
//  AddKey(XK_Menu, 0, "+");

  AddKey(XK_slash, Mod1Mask, "n");
  AddKey(XK_slash, Mod1Mask | ShiftMask, "N");
  AddKey(XK_space, Mod1Mask | ShiftMask,"x");
  AddKey(XK_space, Mod1Mask | ControlMask, " ");
  AddKey(XK_space, Mod1Mask | ControlMask | ShiftMask, "X");
  AddKey(XK_F3,    Mod1Mask, "S");
  AddKey(XK_F3,    Mod1Mask | ShiftMask, "I");
  AddKey(XK_F4,    Mod1Mask, "q");
  AddKey(XK_F4,    Mod1Mask | ShiftMask, "Q");
  AddKey(XK_F5,    Mod1Mask, "c");
  AddKey(XK_F5,    Mod1Mask | ShiftMask, "C");
  AddKey(XK_F6,    Mod1Mask, "m");
  AddKey(XK_F6,    Mod1Mask | ShiftMask, "M");
  AddKey(XK_F7,    Mod1Mask, "D");
  AddKey(XK_F7,    Mod1Mask | ShiftMask, "i");
  AddKey(XK_F8,    Mod1Mask, "\"a ");
  AddKey(XK_F8,    Mod1Mask | ShiftMask, "\"ad");
  AddKey(XK_F9,    Mod1Mask, "\"b ");
  AddKey(XK_F9,    Mod1Mask | ShiftMask, "\"bd");
  AddKey(XK_F10,   Mod1Mask, "\"c ");
  AddKey(XK_F10,   Mod1Mask | ShiftMask, "\"cd");
  AddKey(XK_F11,   Mod1Mask, "\"d ");
  AddKey(XK_F11,   Mod1Mask | ShiftMask, "\"dd");
  AddKey(XK_F12,   Mod1Mask, "\"e ");
  AddKey(XK_F12,   Mod1Mask | ShiftMask, "\"ed");

  AddKey(XK_Tab,   Mod1Mask, "t");
  AddKey(XK_Tab,   Mod1Mask | ShiftMask, "T");
  AddKey(XK_Escape,Mod1Mask,"\x0d"); // ctrl-m
//  AddKey(XK_BackSpace,Mod1Mask, "u");

  AddKey(XK_Left,  Mod1Mask, "h");
  AddKey(XK_Right, Mod1Mask, "l");
  AddKey(XK_Up,    Mod1Mask, "k");
  AddKey(XK_Down,  Mod1Mask, "j");
  AddKey(XK_Page_Up,    Mod1Mask, "a");
  AddKey(XK_Page_Down,  Mod1Mask, "A");
  AddKey(XK_Left,  Mod1Mask | ShiftMask, "H");
  AddKey(XK_Right, Mod1Mask | ShiftMask, "L");
  AddKey(XK_Up,    Mod1Mask | ShiftMask, "K");
  AddKey(XK_Down,  Mod1Mask | ShiftMask, "J");
  AddKey(XK_Left,  Mod1Mask | ControlMask, "\x08");
  AddKey(XK_Right, Mod1Mask | ControlMask, "\x0c");
  AddKey(XK_Up,    Mod1Mask | ControlMask, "\x0b");
  AddKey(XK_Down,  Mod1Mask | ControlMask, "\x0a");
/*}}}*/
  for (int i=HISTSIZE; i;)
    TextDialog::hist[--i] = 0;
  TextDialog::histpos = 0;

}
UEHandler::~UEHandler(){
  XUngrabKey(dpy,AnyKey,AnyModifier,root);
  for (KeyMapIter i = keys.begin();i != keys.end();++i) {
    ModList *ml = i->second,*ml2;
    while (ml) {
      ml2 = ml->next;
      delete ml;
      ml = ml2;
    }
  }
}



void UEHandler::Press(XButtonEvent &e) {
  motion = false;
  SelIcon = 0;
  Client *c = ct->FindClient(e.window, &SelIcon);
  if (c)
    SelDesktop = c->DesktopAbove();
  if (c && c->parent && (e.window == c->window)) {
    if ((e.state & Mod1Mask) && (c->flags & CF_GRABALTCLICK)) {
      if (c->flags & CF_GRABBED)
        XAllowEvents(dpy, AsyncPointer, CurrentTime);
      if (e.state & ControlMask) {
        c = c->parent;
        SelDesktop = SelDesktop->parent;
      }
    } else {
      if (c->flags & CF_GRABBED) {
        if (c->flags & CF_PASSFIRSTCLICK)
          XAllowEvents(dpy, ReplayPointer, CurrentTime);
        else
          XAllowEvents(dpy, AsyncPointer, CurrentTime);
      }
      if (c->flags & CF_FOCUSONCLICK)
        c->GetFocus();
      if (!ISDESKTOP(c)) {
        if (c->flags & CF_RAISEONCLICK) {
          c->Raise(R_PARENT|R_INDIRECT);
        }
      }
    }
  }

  if (c && !((xdown-e.x_root)/2) && !((ydown-e.y_root)/2) && (e.time-downtime < c->Sc->DCTime)) {
    if ((Seltype & T_MOUSEBUTTON) == e.button)
      Seltype = T_DC; else
      Seltype = T_SDC;
  } else Seltype=0;
  if (e.state & ShiftMask)
    Seltype |= T_SHIFT;


  SelClient = c;
  if (c) {

    xdown=e.x_root;
    ydown=e.y_root;
    downtime=e.time;

    bool tmp=false;

    SelEntry=NULL;
    Seltype = e.button | c->GetRegionMask(e.x_root,e.y_root,SelEntry) | (Seltype & (T_DC | T_SDC | T_SHIFT));
    if (SelEntry && SelClient)
      SelClient->ReDrawTBEntry(SelEntry,false);
    if ((Seltype & T_SDC) && SelClient) {
      if (e.state & ShiftMask)
        SelClient->UpdateFlags(FLAG_TOGGLE,CF_AUTOSHADE); else
        SelClient->Shade();
    }
    if (!SelEntry)
      SelEntry = SelClient;
    switch (Seltype) {
      case T_B1 | T_DC | T_F2:
        // it's enough now
        if (SelEntry)
          SelEntry->SendWMDelete(DEL_FORCE);// just kill
        break;
      case T_B1 | T_BAR | T_DC:
        c->Raise(0);
        if (SelEntry)
          SelEntry->RequestDesktop(e.state & Mod1Mask);
        break;
      case T_B2 | T_DC:
      case T_B2 | T_BAR | T_DC:
      {
        Client *cc = SelEntry;
        if (!cc)
          cc = SelClient;
        if (NewTarget) {
          if (cc && cc!=NewTarget)
            NewTarget->target = cc->DesktopAbove(); else
            NewTarget->target=NULL;
          ct->RootDesktop->GetFocus();
          XDefineCursor(dpy, root,ct->RootDesktop->Sc->cursors[CU_STD]);
          NewTarget = NULL;
        } else {
          if (cc) {
            NewTarget = cc;
            XDefineCursor(dpy, root,ct->RootDesktop->Sc->cursors[CU_NEWTARGET]);
          }
        }
      }
      break;
      case T_B3 | T_DC:
      case T_B3 | T_BAR | T_DC:
        c->Lower(L_BOTTOM);
        break;
      case T_B1 | T_DC:
        c->Raise(0);
        c->RequestDesktop(e.state & Mod1Mask);
        break;
      case T_B3 | T_SHIFT:
        tmp = true;
      case T_B2 | T_SHIFT:
      {
        menuitems = new MenuItem[(ct->windows.size()+1)/3];
        int i = 0;
        ct->RootDesktop->GetWindowList(menuitems,i,tmp,xdown,ydown,0,(char *)(tmp ? "P" : "g"));
        if (!i) {
          delete menuitems;
          menuitems = 0;
          break;
        }
        menu = new Menu(0,menuitems,i,0,0,c->Sc,xdown,ydown,true);
        menu->Init();
        break;
      }
      case T_B1:
        if (SelClient->parent)
          break;
      case T_B1 | T_SHIFT:
      {
        MenuInfo * mi = (MenuInfo *)rman->GetInfo(SE_MENUINFO,"");
        if (!mi)
          break;
        menuitems = mi->menu;
        menu = new Menu(0,menuitems,mi->n,0,0,c->Sc,xdown,ydown,true);
        menu->Init();
      }
    }
  }
  if (SelIcon) {
    SetCurrent(0,SelIcon);
    xdown=e.x_root;
    ydown=e.y_root;
    downtime=e.time;
    SelEntry=NULL;
    Seltype=e.button | (Seltype & (T_DC | T_SDC));
  }
}


void UEHandler::Release(XButtonEvent &e) {
  if (MoveWin) {
    XUnmapWindow(dpy,MoveWin);
    XDestroyWindow(dpy,MoveWin);
  }

  if (menu && menu->Remove(0)) {
    delete menu;
    menu = 0;
  }

  if (dialog && e.window == dialog->window) {
    int arg=0;
    if (e.button == Button2) {
      dialog->SetCursor(e.x);
      dialog->Paste();
      return;
    }
    if (e.button == Button1)
      arg = e.x;
    if (e.button == Button3)
      arg = 32767;
    dialog->SetCursor(arg);
    return;
  }

  if (SelIcon && MoveWin) {
    Desktop *d = ct->FindPointerClient()->DesktopAbove();
    bool copy = (e.state & ControlMask);
    if (copy  || d != SelIcon->parent) {
      Client *saveclient = SelIcon->cmd ? 0 : SelIcon->client;
      Icon *i = new Icon(d,SelIcon,copy);
      d->AddIcon(i);
      i->Init();
      if (!copy) {
        SelIcon->parent->RemoveIcon(SelIcon);
        if (saveclient)
          saveclient->icon = i;
      }
    }

  }

  Client *c = SelClient;
  if (c) {
    if (SelEntry && (Seltype == (Button2 | T_BAR)) && MoveWin) {
      Client *tb=0;
      Desktop *d = ct->FindPointerClient()->DesktopAbove();

      // Try to reparent SelEntry under d
      if (d != SelEntry->parent) {
        Desktop *dd;
        for (dd=d;dd;dd=dd->parent)
          if (dd==SelEntry)
            break;
        if (d && !dd) { //Don't try to reparent a desktop under itself
          Desktop *oldparent = SelEntry->parent;
          oldparent->GiveAway(SelEntry,true);
          if (tb)
            d->Take(SelEntry,SelEntry->x,SelEntry->y); else
            d->Take(SelEntry,
              e.x_root - xdown + SelEntry->x_root  -  d->x_root - SelEntry->Sc->BW,
              e.y_root - ydown + SelEntry->y_root  -  d->y_root - SelEntry->Sc->BW);
              /*  movement  */
              /*         absolute position        */
            oldparent->AutoClose();
        }
      }
    }
    Client *t=NULL;
    if (RefClient && Seltype != (T_B2 | T_BAR) && Seltype != T_B2)
      RefClient = 0;
    if ((T_DC | T_SHIFT | Seltype) ==
        (T_DC | T_SHIFT | e.button | c->GetRegionMask(e.x_root,e.y_root,t)))
      if (SelEntry == (t ? t : SelClient)/* && !SelIcon*/)
        switch (Seltype) {
          case T_B1:
            if (!motion)
              c->Raise(0);
            break;
          case T_B1 | T_BAR | T_SHIFT:
            c->UpdateFlags(FLAG_DSET,CF_ABOVE);
          case Button1 | T_BAR:
            {
              if (motion)
                break;
              Client *i;
              for (i=t;i->transfor;i=i->transfor);
              i->Raise((t!=c && !t->MaxH && !t->MaxW) ? (R_MOVEMOUSE | R_GOTO) : R_GOTO);
              t->GetFocus();
            }
            break;
          case T_B2 | T_BAR | T_SHIFT:
            c->UpdateFlags(FLAG_DREMOVE,CF_ABOVE|CF_BELOW);
            break;
          case T_B2:
          case T_B2 | T_BAR:
            if (RefClient) {
              if (RefClient == SelEntry)
                break;
              RefClient->LowerBelow(SelEntry);
              RefClient = 0;

            } else {
              RefClient = SelEntry;
            }
            break;
          case T_B3:
            if (!motion)
              c->Lower(0);
            break;
          case T_B3 | T_BAR | T_SHIFT:
            c->UpdateFlags(FLAG_DSET,CF_BELOW);
          case Button3 | T_BAR:
            {
              if (motion)
                break;
              Client *i;
              for (i=t;i->trans;i=i->trans);
              i->Lower(0);
            }
            break;
          case T_B1 | T_F1 | T_DC:
          case T_B1 | T_F1:
            t->Maximize((e.state & Mod1Mask) ? MAX_VERTFULL : MAX_FULL);
            break;
          case T_B2 | T_F1:
          case T_B2 | T_F1 | T_DC:
            t->Maximize((e.state & Mod1Mask) ? MAX_VERTHALF : MAX_HALF);
            break;
          case T_B3 | T_F1:
	    if (e.state & Mod1Mask)
	      t->Maximize(MAX_HORI); else {
              int oldw = t->width;
              t->MoveResize(S_BOTTOM|S_RIGHT,true,0,0);
              setmouse(root, ((t==c) ? (t->width-oldw) : 0) + e.x_root, e.y_root);
            }
            break;
          case Button1 | T_F2:
            t->SendWMDelete((e.state & Mod1Mask) ? DEL_RELEASE : DEL_NORMAL);
            break;
          case Button2 | T_F2:
            t->Hide();
            if (!t->mapped)
              SelClient = 0;
            break;
          case Button3 | T_F2:
            if (t == c) {
              c->MoveResize(0,false,e.x_root, e.y_root);
            } else {
              t->MoveResize(0,true,0,0);
              setmouse(root, e.x_root, e.y_root);
            }
            break;
          case Button1 | T_F2 | T_SHIFT:
            c->UpdateFlags(FLAG_DTOGGLE,CF_NOCLOSE);
            break;
          case Button2 | T_F2 | T_SHIFT:
            c->UpdateFlags(FLAG_DTOGGLE,CF_NOMINIMIZE);
            break;
          case Button3 | T_F2 | T_SHIFT:
            c->UpdateFlags(FLAG_DTOGGLE,CF_FIXEDSIZE);
            break;

        }

  }

  if (SelIcon && !motion) {
    switch (Seltype) {
      case T_B1:
        SelIcon->Execute();
        break;
      case T_B3:
        SelIcon->parent->ShowIcons(true);
        break;
    }
  }

  MoveWin = 0;
  c = SelClient;
  SelClient = 0;

  if (c && SelEntry)
    c->ReDrawTBEntry(SelEntry,false);

}



void UEHandler::Motion(XMotionEvent &e) {
  if (!motion)
    if (((e.x_root-xdown)/2) || ((e.y_root-ydown)/2))
      motion = true; else
      return;


  if (SelIcon && Seltype == T_B2) {
    if (!MoveWin) {
      XSetWindowAttributes pattr;
      pattr.override_redirect = True;
      pattr.background_pixel = SelIcon->parent->Sc->colors[C_IBG].pixel;
      pattr.border_pixel = SelIcon->parent->Sc->colors[C_IBD].pixel;
      pattr.event_mask = ButtonReleaseMask|ExposureMask;
      pattr.save_under = true;
      MoveWin = XCreateWindow(dpy, root, 0, 0, SelIcon->w, SelIcon->h,
          1, DefaultDepth(dpy, screen), CopyFromParent, DefaultVisual(dpy, screen),
          CWOverrideRedirect|CWBackPixel|CWBorderPixel|CWEventMask|CWSaveUnder, &pattr);

      XMapWindow(dpy,MoveWin);

      XCopyArea(dpy,SelIcon->window,MoveWin,SelIcon->parent->Sc->icon_gc,
        0,0,SelIcon->w,SelIcon->h,0,0);

    }
    XMoveWindow(dpy, MoveWin, e.x_root - SelIcon->w/2, e.y_root - SelIcon->h/2);
    return;
  }

  if (SelIcon && (Seltype == T_B1)) {
    SelIcon->x += e.x_root - xdown;
    SelIcon->y += e.y_root - ydown;
    int oldx=SelIcon->x;
    int oldy=SelIcon->y;
    SelIcon->Move();
    xdown=e.x_root+SelIcon->x-oldx;
    ydown=e.y_root+SelIcon->y-oldy;
    return;
  }

  if (SelIcon && (Seltype == T_B3) && !menu) {
    menu = new Menu(0, IconMenu, IconMenuNum, 0, 0, SelIcon->parent->Sc, xdown,ydown,true);
    menu->Init();
    //icon menu
    return;
  }

// new client menu?
  if (SelClient /*Why???*/ && SelEntry && !menu && (Seltype == (T_B3 | T_BAR) || Seltype == T_B3)) {
    int num;
    if (ISDESKTOP(SelEntry)) {
      OptionMenuInfo.num = DesktopOptionMenuNum;
      num = DesktopMenuNum;
    } else {
      OptionMenuInfo.num = ClientOptionMenuNum;
      num = ClientMenuNum;
    }
    menu = new Menu(0, ClientMenu, num, SelEntry, 0,SelClient->Sc, xdown,ydown,true);
    menu->Init();
  }

  if (menu) {
    menu->Mouse(e.x_root,e.y_root);
    return;
  }

  if (SelClient && Seltype == T_B1)
    if (SelClient->parent) {
      int mode=0;
      if (!(SelClient->flags & CF_FIXEDSIZE)) {
        if (SelClient->width > RESIZEMIN) {
          if (xdown-SelClient->x_root <= SelClient->width/RESIZERATIO &&
              xdown < SelClient->x_root + RESIZEMAX)
            mode |= S_LEFT;
          if (xdown-SelClient->x_root >= (RESIZERATIO - 1)*SelClient->width/RESIZERATIO &&
              xdown >= SelClient->width + SelClient->x_root - RESIZEMAX)
            mode |= S_RIGHT;
        }
        if (SelClient->height > RESIZEMIN) {
          if (ydown-SelClient->y_root <= SelClient->height/RESIZERATIO &&
              ydown < SelClient->y_root + RESIZEMAX)
            mode |= S_TOP;
          if (ydown-SelClient->y_root >= (RESIZERATIO - 1)*SelClient->height/RESIZERATIO &&
              ydown >= SelClient->height + SelClient->y_root - RESIZEMAX)
            mode |= S_BOTTOM;
        }
      }
      if (!mode)
        goto move; // ugly
      SelClient->MoveResize(mode,false,xdown,ydown);
    }

  if (SelDesktop && Seltype == T_B2) {
    SelDesktop->Goto(3*(e.x_root - xdown),3*(e.y_root - ydown),GOTO_RELATIVE | GOTO_SNAP);
    xdown = e.x_root;
    ydown = e.y_root;
  }


  if (SelClient && SelClient->parent && Seltype == (T_BAR | T_B1)) {
    move:
    if (!man->IgnoreLeave) {
      SelClient->x += e.x_root - xdown;
      SelClient->y += e.y_root - ydown;
      int oldx=SelClient->x;
      int oldy=SelClient->y;
      SelClient->Snap(0);
      xdown=e.x_root+SelClient->x-oldx;
      ydown=e.y_root+SelClient->y-oldy;
      xdown -= SelClient->x; // make xdown/down relative to window
      ydown -= SelClient->y;
      man->IgnoreLeave = SelClient->parent->Leave(e.x_root,e.y_root,false);
      oldx = SelClient->x;
      oldy = SelClient->y;
      SelClient->ChangePos(true);
      xdown += oldx;//SelClient->x; // make xdown/ydown absolute
      ydown += oldy;//SelClient->y;
    }
  }



  Client *c=SelClient;

  if (c && SelEntry && SelEntry->parent && Seltype == (T_BAR | T_B2)) {
    int x1=c->x_root - 1 + (SelEntry==SelClient ? SelEntry->TBx : SelEntry->TBEx);
    int y1=c->y_root - 1 - c->THeight;
    if (!MoveWin) {
      XSetWindowAttributes pattr;
      pattr.override_redirect = True;
      pattr.background_pixel = c->Sc->colors[C_TBG].pixel;
      pattr.border_pixel = c->Sc->colors[C_BD].pixel;
      pattr.event_mask = ButtonReleaseMask|ExposureMask;
      pattr.save_under = true;
      int w1 = SelEntry==SelClient ? SelEntry->TBy - SelEntry->TBx :
                                     SelEntry->TBEy - SelEntry->TBEx;
      int h1=c->THeight-1;
      MoveWin = XCreateWindow(dpy, root, x1, y1 + c->THeight, w1, h1,
          1, DefaultDepth(dpy, screen), CopyFromParent, DefaultVisual(dpy, screen),
          CWOverrideRedirect|CWBackPixel|CWBorderPixel|CWEventMask|CWSaveUnder, &pattr);

      XMapWindow(dpy,MoveWin);

      XCopyArea(dpy,c->title,MoveWin,*(c->Sc->title_gc),
        SelEntry == SelClient ? SelEntry->TBx : SelEntry->TBEx,0,w1,h1,0,0);

    }

    XMoveWindow(dpy, MoveWin, x1 + e.x_root-xdown, y1 + e.y_root-ydown);
  }
}


void UEHandler::Key(XKeyEvent &e) {
  if (Current && !dialog && e.window == man->input &&
      ISDESKTOP(Current) && (Current->flags & DF_GRABKEYBOARD)) {
    Current->SendEvent((XAnyEvent &)e);
    return;
  }
  if (e.type != KeyPress)
    return;
  if (menu) {
    if (!menu->Key(e)) {
      menu->Remove(0);
      delete menu;
      menu = 0;
    }
    return;
  }

  KeyMapIter iter = keys.find(e.keycode);
  char *cmd = 0;
  if (iter != keys.end()) {
    for (ModList *ml=iter->second; ml; ml=ml->next) {
      if (e.state == ml->mod) {
        cmd = ml->cmd;
        break;
      }
    }
  }

  if (cmd)
    ExecCommand(0,0,cmd,true); else
      if (dialog) {
	char *line = dialog->Key(e);
	if (line) {
	  delete dialog;
	  dialog = 0;
	  ExecCommand(0,0,line,true);
	  free(line);
	  return;
	}
	if (!(e.state & Mod1Mask))
	  return;
      }

}


void UEHandler::SetCurrent(Client *c, Icon *i) {
  Client *oldCurrent = Current;
  Current = c;
  if (oldCurrent)
    oldCurrent->Clear();
  XSetWindowAttributes attr;
  if (CurrentIcon) {
    attr.background_pixel = CurrentIcon->parent->Sc->colors[C_IBG].pixel;
    XChangeWindowAttributes(dpy, CurrentIcon->window,CWBackPixel,&attr);
    XClearWindow(dpy,CurrentIcon->window);
    CurrentIcon->ReDraw();
  }
  CurrentIcon = i;
  if (!i)
    return;
  attr.background_pixel = i->parent->Sc->colors[C_HIBG].pixel;
  XChangeWindowAttributes(dpy, i->window,CWBackPixel,&attr);
  XClearWindow(dpy,i->window);
  i->ReDraw();
}


void UEHandler::ExecTextCommand(Client *ref,char *str, bool recursive) {
  char command[20];
  char arg1[20];
  char arg2[256];
  int k = sscanf(str,":%19s %19s %255[^\n]",command,arg1,arg2);
  if (!k)
    return;
  lower(command);
  lower(arg1);
  if (ref) {
    int act = rman->GetFlagAct(command);
    if (act) {
      if (FLAG_CMIN <= act && act <=FLAG_CMAX)
        ref = ref->DesktopAbove();
      unsigned long flag = rman->GetFlag(arg1);
      ref->UpdateFlags(act,flag);
      Desktop *d = ref->DesktopAbove();
      if (ref != d && !recursive)
        d->UpdateFlags(act,flag & DF_ALL);
    }
  }
  if (!strcmp("bind",command)) {
    KeySym ks = 0,ks2;
    unsigned int mods = 0;
    char *p = arg1;
    char *p2;
    while (*p) {
      p2 = strchr(p, '+');
      if (p2)
        *p2='\0';
      ks2 = XStringToKeysym(p);
      if (ks2) {
        ks = ks2;
      } else {
        for (int i=0; ModNames[i]; ++i) {
          if (!strcasecmp(ModNames[i],p)) {
            mods |= ModValues[i];
            break;
          }
        }
      }
      if (!p2)
        break;
      p = p2 + 1;
    }
    if (ks)
      AddKey(ks,mods,(k>2) ? strdup(arg2) : 0);


    return;
  }

  if (!strcmp("exit",command) || !strcmp("quit",command)) {
    man->alive = false;
    return;
  }


}

void UEHandler::ExecCommands(char *str) {
  while (ExecCommand(0,0,str,false))
    ++str;
  ExecCommand(0,0,str,true);

}



// This function simplifies much since it is used both to parse user's input
// and to execute internally commands

bool UEHandler::ExecCommand(Client *ref, Icon *i,char *str,bool ExecAll,bool recursive) {
  if (!str)
    return false;
  if (!ref && !i) {
    ref = Current;
    i = CurrentIcon;
  }
  switch (str[0]) {
    case '+':
      if (dialog) {
	if (str[1] == '=') {
	  dialog->SetLine(str + 1);
	} else {
	  delete dialog;
	  dialog = 0;
	}
      } else {
	dialog = new TextDialog((Current ? Current : CurrentIcon->parent)->Sc);
	if (str[1]) {
	  dialog->SetLine(str + 1);
	}
      }
      return true;
    case '/':
      if (!ExecAll)
        return false;
      {
        if (preg)
          regfree(preg);
        preg = new regex_t;
        regcomp(preg,str+1,REG_NOSUB|REG_ICASE|REG_EXTENDED);
      }
    case 'N':
    case 'n':
      if (!preg)
        return true;
      {
        if (!ref)
          ref = ct->Focus ? ct->Focus : ct->RootDesktop;
        int dummy = 0;
        Client *cc = (str[0]=='n') ? ref->Next(false) : ref->Prev(false);
        bool first = true;
        for (Client *c=cc;first || c!=cc; c=(str[0]=='n') ? c->Next(false) : c->Prev(false)) {
          if (!regexec(preg,c->name ? c->name : c->wmname,0,0,dummy)) {
            c->Raise(R_PARENT | R_INDIRECT | R_GOTO | R_MOVEMOUSE);
            return true;
          }
          first = false;
        }
        return true;
      }
    case '!':
      if (!ExecAll)
        return false;
      Fork(str+1);
      return true;
    case '"':
      if (!ExecAll)
        return false;
      {
        char r = tolower(str[1]);
        if (!('a'<=r && r<='z') && r!='\'')
          return true;
        if (str[2] == 'd' || str[2] == 'y') {
          reg[r-'a'] = (r=='\'') ? ct->RootDesktop : ref;
          return true;
        }
        dreg = (r=='\'') ? ct->RootDesktop : reg[r-'a'];
        ExecCommand(dreg,0,str+2,ExecAll);
        return true;
      }
    case '@':
      if (!ExecAll)
        return false;
      {
        Action *ac = ((Action *)rman->GetInfo(SE_ACTION,str+1));
        if (ac)
          ac->Execute();
      }
      return true;
    case '$':
      if (!ExecAll)
        return false;
      {
        MenuInfo *mi = ((MenuInfo *)rman->GetInfo(SE_MENUINFO,str+1));
        if (mi) {
          int x,y;
          GetMousePosition(root, x, y);
          menu = new Menu(0,mi->menu,mi->n,0,0,ct->RootDesktop->Sc,x,y,true);
          menu->Init();
        }
      }
      return true;
    case ':':
      if (!ExecAll)
        return false;
      ExecTextCommand(ref,str,recursive);
      return true;
    case 'R':
      if (!ExecAll)
        return false;
      ref = ct->RootDesktop;
      str[0]='r';
      break;
#ifdef DEBUG
    case '3':
      ct->RootDesktop->Remove(R_RESTART);
      ct->RootDesktop->Init();
      return true;
#endif
  };
  if (ref)
    switch (str[0]) {
      case '?':
	{
	  if (menu)
	    return true;
	  int num;
	  if (ISDESKTOP(ref)) {
	    OptionMenuInfo.num = DesktopOptionMenuNum;
	    num = DesktopMenuNum;
	  } else {
	    OptionMenuInfo.num = ClientOptionMenuNum;
	    num = ClientMenuNum;
	  }
	  GetMousePosition(root,xdown,ydown);
	  menu = new Menu(0, ClientMenu, num,
	      SelEntry,0,ref->Sc,xdown,ydown-3,true); //!!!
	  menu->Init();
	  return true;
	}
      case 'y':
      case 'd':
        dreg = ref;
        return true;
      case ' ':
        ref->Raise(R_INDIRECT);
        return true;
      case 'P':
        ref->Raise(R_INDIRECT | R_PARENT);
        return true;
      case 'g':
        ref->Raise(R_INDIRECT | R_GOTO | R_MOVEMOUSE);
        return true;
      case 'S':
        ref->Shade();
        return true;
      case 's':
        ref->flags ^= CF_STICKY;
        return true;
      case 'f':
      case 0x0d: // Ctrl-M
        ref->Maximize(MAX_FULLSCREEN);
        return true;
      case 'm':
        ref->Maximize(MAX_HALF);
        return true;
      case 'M':
        ref->Maximize(MAX_FULL);
        return true;
      case 'h':
      case 'H':
        {
          Client *c = ref->Prev(true);
          if (!c)
            return true;
	  if (ct->MaxWindow == ref)
	    c->Maximize(MAX_FULLSCREEN);
          c->GetFocus();
          if (str[0] == 'H')
            c->Raise(R_GOTO); else
            c->Scroll(true);
          return true;
        }
      case 0x08: // Ctrl-H
        ref->MoveLeft();
        return true;
      case 'l':
      case 'L':
        {
          Client *c = ref->Next(true);
          if (!c)
            return true;
          c->GetFocus();
	  if (ct->MaxWindow == ref)
	    c->Maximize(MAX_FULLSCREEN);
          if (str[0] == 'L')
            c->Raise(R_GOTO); else
            c->Scroll(true);
          return true;
        }
      case 0x0c: // Ctrl-L
        ref->MoveRight();
        return true;
      case 'k':
      case 'K':
        {
          Client *c = ref->parent;
          if (!c)
            return true;
	  if (ct->MaxWindow == ref)
	    c->Maximize(MAX_FULLSCREEN);
          c->GetFocus();
          if (str[0] == 'K')
            c->Raise(R_GOTO); else
            c->Scroll(true);
          return true;
        }
      case 0x0b: // Ctrl-K
        ref->MoveUp();
        return true;
      case 'j':
      case 'J':
        {
          Desktop *d =ref->DesktopAbove();
          Client *c = d->focus ? d->focus : d->firstchild;
          if (!c)
            return true;
	  if (ct->MaxWindow == ref)
	    c->Maximize(MAX_FULLSCREEN);
          c->GetFocus();
          if (str[0] == 'J')
            c->Raise(R_GOTO); else
            c->Scroll(true);
          return true;
        }
      case 0xa: // Ctrl-J
        ref->MoveDown();
        return true;
      case 'i':
        ref->DesktopAbove()->ShowIcons(true);
        return true;
      case 'I':
        ref->Hide();
        return true;
      case 'Q':
        if (!ISDESKTOP(ref))
          ref->SendWMDelete(DEL_FORCE);
        return true;
      case 'q':
        ref->SendWMDelete(DEL_NORMAL);
        return true;
      case 'e':
      case 0x11: // Ctrl-Q
        ref->SendWMDelete(DEL_RELEASE);
        return true;
      case 'c':
        ref->MoveResize(0,true,0,0);
        return true;
      case 'C':
        ref->MoveResize(S_RIGHT | S_BOTTOM,true,0,0);
        return true;
      case 'D':
        ref->RequestDesktop(false);
        return true;
      case 'E':
      case 0x04: // Ctrl+D
        ref->DesktopAbove()->RequestDesktop(true);
        return true;
      case 'x':
        ref->Lower(0);
        return true;
      case 'X':
        ref->Lower(L_PARENT);
        return true;
      case 0x18: // Ctrl-X
        ref->Lower(L_BOTTOM);
        return true;
      case 'p':
        {
          Desktop *d = ref->DesktopAbove();
          // Try to reparent dreg under d
          if (!dreg || d == dreg->parent)
            return true;
          Desktop *dd;
          for (dd=d;dd;dd=dd->parent)
            if (dd==dreg)
              break;
          if (d && !dd) { //Don't try to reparent a desktop under itself
            Desktop *oldparent = dreg->parent;
            oldparent->GiveAway(dreg,true);
            d->Take(dreg,dreg->x,dreg->y);
            oldparent->AutoClose();
          }
          return true;
        }
      case '=':
        if (!ExecAll)
          return false;
        {
          Desktop *d = ref->DesktopAbove();
          if (d->name)
            free(d->name);
          if (str[1])
            d->name = strdup(str+1); else
            d->name = 0;
          // Surprisingly, this works
          (d->focus ? d->focus : d)->UpdateName(false);
          d->Clear();

        }
        return true;
      case 'a':
        ref->Stacking(true);
        return true;
      case 'A':
        ref->Stacking(false);
        return true;
      case 't':
        {
          Client *c = ref->Focus();
          c = c->Next(false);
          if (c) {
            c->Raise(R_MOVEMOUSE | R_GOTO | R_PARENT);
            c->GetFocus();
	    if (ct->MaxWindow == ref)
	      c->Maximize(MAX_FULLSCREEN);
          }
        }
        return true;
      case 'T':
        {
          Client *c = ref->Focus();
          c = c->Prev(false);
          if (c) {
            c->Raise(R_MOVEMOUSE | R_GOTO | R_PARENT);
            c->GetFocus();
          }
        }
        return true;
      case 'R':
      case 'r':
        if (!ExecAll)
          return false;
        ExecCommand(ref,i,str+1,true);
        {
          Desktop *d = dynamic_cast<Desktop *>(ref);
          if (!d)
            return true;
          for (Client *c=d->firstchild;c;c=c->next)
            ExecCommand(c,i,str,true,true);
        }
        return true;
      case 'o':
        ref->MakeHole(true);
        return true;
      case 'O':
        ref->MakeHole(false);
        return true;
      case '-':
        ref = ref->DesktopAbove();
        ref->UpdateFlags((ref->mask & DF_GRABKEYBOARD) ? FLAG_CREMOVE : FLAG_CUNSET,CF_FOCUSONENTER);
        ref->flags ^= DF_GRABKEYBOARD;
        ref->mask |=  DF_GRABKEYBOARD;
        ref->GetFocus();
        return true;
#ifdef DEBUG
      case '1':
        {
          Pixmap pm = XCreatePixmap(dpy,root,200,200,1);
          XShapeCombineMask(dpy, ref->frame, ShapeBounding,
            0, ref->THeight, pm, YXBanded);
          ref->flags |= CF_HASBEENSHAPED;
          XFreePixmap(dpy,pm);
        }
        return true;
      case '2':
        ref->Remove(R_RESTART);
        ref->Init();
        return true;
#endif

      default:
        return false;
    }

  if (i)
    switch (str[0]) {
      case '?':
	  GetMousePosition(root,xdown,ydown);
	  menu = new Menu(0, IconMenu, IconMenuNum, 0, 0, i->parent->Sc,xdown,ydown-3,true); //!!!
	  menu->Init();
	  return true;
      case ' ':
        i->Execute();
        return true;
      case 'h':
        i->parent->NextIcon(i,NI_LEFT);
        return true;
      case 'l':
        i->parent->NextIcon(i,NI_RIGHT);
        return true;
      case 'j':
        i->parent->NextIcon(i,NI_DOWN);
        return true;
      case 'k':
        i->parent->NextIcon(i,NI_UP);
        return true;
      case 't':
      case 'T':
        SetCurrent(ct->Focus ? ct->Focus : ct->RootDesktop,0);
        Current->Raise(R_MOVEMOUSE | R_GOTO | R_PARENT);
        return true;
      case '=':
        if (!ExecAll)
          return false;
        i->ChangeName(str+1);
        return true;
      case 'q':
        if (i->client && !i->cmd)
          i->client->Map(); else
          i->parent->RemoveIcon(i);
        return true;
      case '+':
        if (!ExecAll)
          return false;
        if (i->cmd)
          free(i->cmd); else
          if (i->client)
            return true;
          i->cmd = strdup(str+1);
        return true;
    }

  return false;
}

void UEHandler::RemoveClientReferences(Client *c) {
  if (NewTarget == c) {
    XDefineCursor(dpy, root,ct->RootDesktop->Sc->cursors[CU_STD]);
    NewTarget = NULL;
  }
  if (SelEntry == c)
    SelEntry = 0;
  if (SelClient == c)
    SelClient = 0;
  if (SelDesktop == c)
    SelDesktop = 0;
  if (Current == c)
    Current = ct->RootDesktop;
  if (RefClient == c)
    RefClient = 0;
  for (int i = 'z'-'a';i+1;--i)
    if (c == reg[i])
      reg[i] = 0;
}

/* treewm - an X11 window manager.
 * Copyright (c) 2001-2003 Thomas Jäger <TheHunter2000 at web dot de>
 * This code is released under the terms of the GNU GPL.
 * See the included file LICENSE for details.
 *
 * changed<05.02.2003> by Rudolf Polzer: corrected initialization of root
 *   window's dimensions in Client::Init.
 *   Added debugging event output in MoveMouse
 */

#include "client.h"
#include "clienttree.h"
#include "sceme.h"
#include "clientinfo.h"
#include "icon.h"
#include "MwmUtil.h"
#include "textdialog.h"
#include <X11/Xmd.h>
#include <string.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#ifdef VIDMODE
#include <X11/extensions/xf86vmode.h>
#endif


void GetMousePosition(Window w, int &x, int &y) {
  Window mouse_root, mouse_win;
  int win_x, win_y;
  unsigned int mask;

  XQueryPointer(dpy, w, &mouse_root, &mouse_win,
      &x, &y, &win_x, &win_y, &mask);
}

void FetchName(Window window, char **name) {
  XTextProperty text_prop;
  char** cl;
  int n;

  if (XGetWMName(dpy, window, &text_prop) != 0) {
    if (text_prop.encoding == XA_STRING) {
      *name = (char *)text_prop.value;
    } else {
      XmbTextPropertyToTextList(dpy, &text_prop, &cl, &n);
      if (cl) {
	*name = strdup(cl[0]);
	XFreeStringList(cl);
      } else {
	*name = NULL;
      }
    }
  } else {
    *name = NULL;
  }
}

void Client::Clear() {
  if (flags & CF_IGNOREEXPOSE)
    return;
  flags |= CF_IGNOREEXPOSE;
  if (visible)
    man->AddReDrawClient(this);
}

unsigned long Client::GetRegionMask(int x, int y, Client * &t) {
  x -= x_root;
  y += THeight - y_root;
  if (0 <= y && y <= THeight && 0 <= x && x <= width) {
    t = this;
    if (!(flags & CF_TITLEBARBUTTONS))
      return T_BAR;
    return ((x >= TBy - ((flags & CF_FIXEDSIZE) ? 1 : 2)*BUTTONWIDTH) ? (x >= TBy - BUTTONWIDTH) ? T_F2 : T_F1 : T_BAR);
  }
  return 0;
}


Client::Client(Desktop *p, Window win) {
  parent = p;
  flags = DEFAULTFLAGS;
  parentmask = 0;
  mask = DEFAULTMASK;
  window = win;
  wmname =  ct->notitle;
  name = 0;
  Sc = parent ? parent->Sc : 0;
  iconpm = 0;
  x = UNDEF;
  y = UNDEF;
  width = UNDEF;
  height = UNDEF;
  GetFlags();
  if (window)
    GetMWMOptions();
  ignoreunmap = 0;
  MaxX = 0;MaxW = 0;
  MaxY = 0;MaxH = 0;
  target = 0;
  trans = 0;
  transfor = 0;
  icon = 0;
}

void Client::SetClientInfo(ClientInfo *ci) {
  if (!ci)
    return;
  if (ci->Sc)
    Sc = ci->Sc;
  if (ci->iconpm)
    iconpm = ci->iconpm; // We do not need to reserve extra memory
  if ((ci->flags & CI_ICONX) && (ci->flags & CI_ICONY)) {
    IconX = ci->IconX;
    IconY = ci->IconY;
    flags |= CF_ICONPOSKNOWN;
  }
  mask |= ci->clientmask;
  flags &= ~ci->clientmask;
  flags |= ci->clientflags;
  if (parent) {
    if (ci->flags & CI_X)
      x = ci->x;
    if (ci->flags & CI_Y)
      y = ci->y;
    if (ci->flags & CI_WIDTH)
      width = ci->width;
    if (ci->flags & CI_HEIGHT)
      height = ci->height;
  }

}

// it is important that everything's correct by the time this function is called
bool Client::Init() {
  XWindowAttributes attr;

  long dummy;
  XGrabServer(dpy);
// Get options
  if (!Sc && parent)
    Sc = parent->Sc;
  if (!Sc)
    Sc = (Sceme *)rman->GetInfo(SE_SCEME,"");

  if (!ISDESKTOP(this)) {
    Window tfor;
    XGetTransientForHint(dpy, window, &tfor);
    if (tfor) {
      transfor = ct->FindClient(tfor, 0);
      if (transfor) {
        for (;transfor->trans;transfor=transfor->trans);
        transfor->trans=this;
      }
    }
  }
  ChangeName();
  frame = 0;
  title = 0;
  size = 0;
  if (parent && XGetWindowAttributes(dpy, window, &attr) != 1)
    return false;

  if (flags & CF_HASTITLE) {
    XFontSetExtents *e;
    e = XExtentsOfFontSet(Sc->fonts[FO_STD]);
    THeight= e->max_logical_extent.height*4/5 + e->max_logical_extent.height/5 + 2*SPACE;
  } else
    THeight=0;


  cmap = attr.colormap;
  size = XAllocSizeHints();

  if (parent) {
    XGetWMNormalHints(dpy, window, size, &dummy);
    if ((size->flags & PMinSize) && (size->flags & PMaxSize) &&
        (size->min_width==size->max_width) && (size->min_height==size->max_height)) {
      flags |= CF_FIXEDSIZE;
    }
  } else {
    flags |= CF_FIXEDSIZE;
  }

  mapped = (attr.map_state == IsViewable);
  if (parent) {
    if (mapped) {
      if (width == UNDEF)
        width = attr.width;
      if (height == UNDEF)
        height = attr.height;
      if (attr.x == 0 && attr.y == 0) { //started without window manager running
        if (x == UNDEF)
          x = Sc->BW;
        if (y == UNDEF)
          y = x + THeight;
      } else {
        if (x == UNDEF) {
          x = attr.x;
          x -= parent->x_root;
        }
        if (y == UNDEF) {
          y = attr.y;
          y -= parent->y_root;
        }
      }
    } else {
      if (width == UNDEF)
        width = attr.width;
      if (height == UNDEF)
        height = attr.height;
      if (x == UNDEF || y == UNDEF) {
        InitPosition();
        Snap(0);
      }
    }
  } else {
    x=0;
    y=0;
    width = DWidth - 2*Sc->BW;
    height = DHeight - 2*Sc->BW - THeight;
  }
  Gravitate(1);
  Reparent();

  if (THeight)
    XMapWindow(dpy, title);
  if (!icon) {
    if (window) {
      XMapWindow(dpy, window);
      XMapWindow(dpy, frame);
    }
    mapped = true;
    visible = true;
  }

  SetWMState();

  UpdateTB();
  if (parent && !parent->focus) {
    parent->focus = this;
    UpdateName(false);
  }
  Raise(R_INDIRECT);
  GrabButtons(true);
  if (window) {
    ct->windows.insert(WmapPair(window,this));
    ct->windows.insert(WmapPair(frame,this));
  }
  ct->windows.insert(WmapPair(title,this));


#ifdef DEBUG
  dump();
#endif
  XSync(dpy, False);
  XUngrabServer(dpy);


  return true;
}

Client* Client::FindPointerClient() {
  return this;
}

bool Client::GetWindowList(MenuItem *m,int &i,bool mouse,int mx,int my,int ind,char *key) {
  if (!mapped && !icon)
    return false;
  if (mouse &&!(x_root <= mx && mx <= x_root+width &&
                y_root-THeight <= my &&  my <= y_root+height))
    return false;
  m[i].text = name ? name : wmname;
  if (!m[i].text)
    m[i].text = "";
  m[i].key = key;
  m[i].client = this;
  m[i].flags = ind * MF_INDENT;
  m[i].submenu = 0;
  ++i;
  return true;
}

void Client::GetFlags() {
  unsigned long todo = ~(mask | parentmask);
  flags &= (mask | parentmask);

  Desktop *d = parent;
  while (d && todo) {
    flags |= (todo & d->childmask & d->childflags);
    todo &= ~(d->childmask);
    flags |= (todo & ~d->parentmask & d->flags);
    todo &= d->parentmask;
    d = d->parent;
  }
  flags |= (todo & DEFAULTFLAGS);

}


void Client::UpdateFlags(int act,unsigned long flag) {
  unsigned long changeflags = flags;
  switch (act) {
    case FLAG_DREMOVE:
      parentmask &= ~flag;
      GetFlags();
      break;
    case FLAG_REMOVE:
      mask &= ~flag;
      GetFlags();
      break;
    case FLAG_UPDATE:
      GetFlags();
      break;
    case FLAG_DSET:
      parentmask |= flag;
      flags |= flag;
      break;
    case FLAG_SET:
      flags |= flag;
      mask |= flag;
      break;
    case FLAG_DUNSET:
      parentmask |= flag;
      flags &= ~flag;
      break;
    case FLAG_UNSET:
      flags &= ~flag;
      mask |= flag;
      break;
    case FLAG_DTOGGLE:
      parentmask |= flag;
      flags ^= flag;
      break;
    case FLAG_TOGGLE:
      flags ^= flag;
      mask |= flag;
      break;
    default:
      return;
  }
  changeflags ^= flags;
  if (changeflags & CF_HASTITLE) {
    flags ^= CF_HASTITLE;
    ToggleHasTitle();
  }
  if (changeflags & CF_HIDEMOUSE) {
    XDefineCursor(dpy,window,(flags & CF_HIDEMOUSE) ? Sc->cursors[CU_NONE] : None);
  }
  if (parent && (changeflags & CF_HASTBENTRY))
    parent->UpdateTB();
  if (changeflags & (CF_RAISEONCLICK | CF_FOCUSONCLICK | CF_GRABALTCLICK))
    GrabButtons(true);
  if (changeflags & (DF_TASKBARBUTTONS | CF_TITLEBARBUTTONS))
    Clear();

}


void Client::RemoveClientReferences(Client *c) {
  if (c==target)
    target = 0;
}


/* MwmHints are so crappy! */
void Client::GetMWMOptions() {
  if (ISDESKTOP(this))
    return;
  Atom real_type; int real_format;
  unsigned long items_read, items_left;
  PropMwmHints *mhints;
  if (XGetWindowProperty(dpy, window, mwm_hints, 0L, 20L, False,
      mwm_hints, &real_type, &real_format, &items_read, &items_left,
      (unsigned char **) &mhints) == Success
      && items_read >= PROP_MOTIF_WM_HINTS_ELEMENTS) {
// decorations (no title is not ignored, configure requests are treated differently, thank you freeamp)
    if (mhints->flags & MWM_HINTS_DECORATIONS &&
        !(mhints->decorations & MWM_DECOR_ALL) &&
        !(mhints->decorations & MWM_DECOR_TITLE)) {
      flags |= CF_FORCESTATIC;
    }

    if (mhints->flags & MWM_HINTS_FUNCTIONS &&
        !(mhints->functions & MWM_FUNC_ALL)) {
      if (!(mhints->functions & MWM_FUNC_RESIZE)) {
        flags |= CF_FIXEDSIZE;
        flags |= CF_FIXEDSIZE;
      }
      if (!(mhints->functions & MWM_FUNC_MINIMIZE)) {
        flags |= CF_NOMINIMIZE;
        mask |= CF_NOCLOSE;
      }
      if (!(mhints->functions & MWM_FUNC_CLOSE)) {
        flags |= CF_NOCLOSE;
        mask |= CF_NOCLOSE;
      }
    }


    XFree(mhints);
  }
}


Desktop* Client::DesktopAbove() {
  return parent;
}


Client* Client::Next(bool thisdesktop) {
  if (thisdesktop) {
    if (!parent)
      return 0;
    Client *n=this;
    do {
      n = n->next ? n->next : (parent ? parent->firstchild : 0);
    } while (n && !n->mapped && n!=this);
    if (n == this)
      return 0;
    return n;
  } else {
// it's funny how difficult this is if not solved recursively
    Desktop *d;
    Client *n;
    if (parent) {
      d = parent;
      n = next;
    } else {
      d = DesktopAbove();
      n = d->firstchild;
    }
    int RD = 2;
    for (;RD;) {
      if (n) {
        if (n->mapped) { // go down
          d = n->DesktopAbove();
          if (n!=d)
            return n; // this is no desktop
          n = d->firstchild;
        } else n = n->next; // go right
      } else { // go up + right
        if (d) {
          if (d->next) {
            n = d->next;
            d = d->parent;
          } else {
            d = d->parent;
            n = 0;
          }
        } else {
          n = ct->RootDesktop;
          --RD;
        }
      }
    }
  }
  return 0;
}

Client* Client::Prev(bool thisdesktop) {
  if (thisdesktop) {
    if (!parent)
      return 0;
    Client *prev = 0;
    for (Client *i = parent->firstchild;i!=this;i=i->next)
      if (i->mapped)
        prev = i;
    if (prev)
      return prev;
    for (Client *i = this;i;i=i->next)
      if (i->mapped)
        prev = i;
    return prev;
  } else {
// this is even more difficult
    Desktop *d;
    Client *p = 0,*c; // previous, current
    if (parent) {
      d = parent;
      c = this;
    } else {
      d = DesktopAbove();
      c = d->firstchild;
    }
    for (Client *i = d->firstchild;i!=c;i=i->next) // move left
      if (i->mapped)
        p = i;
    c = p;
    int RD = 2;
    for (;RD;) {
      if (c) {
        d = c->DesktopAbove();
        if (c!=d)
          return c;
        c = 0;
        p = 0;
      } else {
        if (d && d->parent) {
          c = d;
          d = d->parent;
        } else {
          d = ct->RootDesktop;
          --RD;
          c = 0;
        }
      }
//    p == 0;
      if (d) {
        for (Client *i = d->firstchild;i && i!=c;i=i->next) // find last window in desktop
          if (i->mapped)
            p = i;
      }
      c = p;
    }

  }
  return 0;
}

void Client::MoveLeft() {
  if (!parent)
    return;
// find client to put me after
  Client *newprev = 0,*newprev2=0;
  for (Client *i=parent->firstchild;i!=this;i=i->next) // must find me in list
    if (i->mapped && (i->flags & CF_HASTBENTRY)) {
      newprev = newprev2;
      newprev2 = i;
    }
// remove me
  if (parent->firstchild == this) {
    parent->firstchild = next;
  } else {
    Client *prev = parent->firstchild;
    for (;prev->next != this;prev=prev->next); // must find me in the list
    prev->next = next;
  }
// insert me
  if (newprev2) { // is there at least one client before me?
    if (newprev) { // are there at least two?
      Client *newnext = newprev->next;
      newprev->next = this;
      next = newnext;
    } else {
// i must be the firstchild
      next = parent->firstchild;
      parent->firstchild = this;
    }
  } else {
// i must be the last child
    Client *last = parent->firstchild;
    if (last) {
      for (;last->next;last=last->next);
      last->next = this;
    } else {
      parent->firstchild = this;
    }
    next = 0;
  }
  parent->UpdateTB();

}

void Client::MoveRight() {
  if (!parent)
    return;
// remove me
  if (parent->firstchild == this) {
    parent->firstchild = next;
  } else {
    Client *prev = parent->firstchild;
    for (;prev->next != this;prev=prev->next); // must find me in the list
    prev->next = next;
  }
// find client to put me after
  Client *newprev = next;
  for (;newprev;newprev = newprev->next)
    if (newprev->mapped && (newprev->flags & CF_HASTBENTRY))
      break;
// insert me
  if (newprev) {
    Client *newnext = newprev->next;
    newprev->next = this;
    next = newnext;
  } else {
// i must be the firstchild
    next = parent->firstchild;
    parent->firstchild = this;
  }

  parent->UpdateTB();
}

void Client::MoveUp() {
  if (!parent || !parent->parent)
    return;
  Desktop *p = parent;
  parent->GiveAway(this,true);
  parent->parent->Take(this,x + parent->x_root - parent->parent->x_root,
                            y + parent->y_root - parent->parent->y_root);
  p->AutoClose();
  man->WantFocus = this;
}

void Client::MoveDown() {
  if (!parent)
    return;
  Client *n=this;
  Desktop *d=0;
  do {
    n = n->next ? n->next : (parent ? parent->firstchild : 0);
    d = n ? n->DesktopAbove() : 0;
  } while (n && (!n->mapped || (n!=this && d!=n)));
  if (!d || n==this)
    return;
  parent->GiveAway(this,true);
  d->Take(this,x,y);
  man->WantFocus = this;
}


void Client::Stacking(bool up) {
  if (!parent)
    return;
  if (up) {
    switch (flags & (CF_BELOW | CF_ABOVE)) {
      case (CF_BELOW | CF_ABOVE):
        flags &= ~CF_BELOW;
        break;
      case 0:
        flags |= CF_ABOVE;
        break;
      case CF_ABOVE:
        return;
      case CF_BELOW:
        flags &= ~CF_BELOW;
        break;
    }
  } else {
    switch (flags & (CF_BELOW | CF_ABOVE)) {
      case (CF_BELOW | CF_ABOVE):
        flags &= ~CF_ABOVE;
        break;
      case 0:
        flags |= CF_BELOW;
        break;
      case CF_ABOVE:
        flags &= ~CF_ABOVE;
        break;
      case CF_BELOW:
        return;
    }
  }
  mask |= CF_BELOW | CF_ABOVE;
  parentmask |= CF_BELOW | CF_ABOVE;
  for (int i=0;i!=3;++i)
    parent->LastRaised[i] = 0;
  Raise(0);
}

int Client::GetStacking() {
  if (flags & CF_ABOVE)
    return 2;
  if (flags & CF_BELOW)
    return 0;
  return 1;
}

void Client::Raise(int mode) {
  if (!parent)
    return;
  if (!mapped) {
    if (mode & R_MAP)
      Map(); else
      return;
  }
  int stacking = GetStacking();
  if (mapped && (parent->LastRaised[stacking] != this))  {
    Client *c = parent->LastRaised[stacking];
    parent->LastRaised[stacking] = this;
    GrabButtons();
    if (c)
      c->GrabButtons();
    if (stacking == 2 && parent->parent) {
      XRaiseWindow(dpy, frame);
    } else {
      XWindowChanges wc;
      wc.sibling = (stacking==2) ? parent->title :
                                   parent->StackRef[stacking+1];
      wc.stack_mode = Below;
      XConfigureWindow(dpy,frame,CWSibling | CWStackMode,&wc);
    }
  }
  if (trans) {
    if (trans->mapped)
      trans->Raise(mode);
  } else {
    if (mode & R_PARENT)
      parent->Raise(R_INDIRECT | R_PARENT);
    if (mode & R_GOTO)
      Scroll(mode & R_MOVEMOUSE);
  }
}

void Client::Scroll(bool MoveMouse) {
  if (!parent || ct->MaxWindow)
    return;
  parent->Scroll(false);
  parent->Goto(x + width/3,y + height/3,0);
  if (!MoveMouse)
    return;
  if (MaxW || MaxH)
    setmouse(parent->frame,(TBEx+TBEy)/2,THeight/2); else
    setmouse(frame,(TBx+TBy)/2,THeight/2);
}

void Client::Lower(unsigned int mode) {
  if (!parent)
    return;
  int stacking = GetStacking();
  if (mapped) {
    if ((mode & L_BOTTOM) || !(~flags & (CF_BELOW | CF_ABOVE))) {
      XLowerWindow(dpy,frame);
    } else {
      XWindowChanges wc;
      wc.sibling = parent->StackRef[stacking];
      wc.stack_mode = Above;
      XConfigureWindow(dpy,frame,CWSibling | CWStackMode,&wc);
    }
  }
  if (parent->LastRaised[stacking] == this) {
    GrabButtons();
    parent->LastRaised[stacking] = 0;
  }
  if (mapped)
    GrabButtons();
  if (transfor) {
    transfor->Lower(mode);
  } else {
    if (mode & L_PARENT)
      parent->Lower(mode);
  }
}

/* This function restacks the window below c
 * if both windows don't have the same parent the first ancestors
 * with the same parent are used
 */
void Client::LowerBelow(Client *c) {
  if (!parent || !c || !c->parent)
    return;
  Client *i = this;
  if (i->parent != c->parent) {
    ClientList *tmp;
    ClientList *il=0;
    ClientList *cl=0;
// Create two stacks (linked lists, top is first element)
    for (;c;c=c->parent) {
      tmp = cl;
      cl = new ClientList;
      cl->next = tmp;
      cl->client = c;
    }
    for (;i;i=i->parent) {
      tmp = il;
      il = new ClientList;
      il->next = tmp;
      il->client = i;
    }
    while (il && cl && il->client==cl->client) {
      tmp = il;
      il = il->next;
      delete tmp;
      tmp = cl;
      cl = cl->next;
      delete tmp;
    }
    i = il ? il->client : 0;
    c = cl ? cl->client : 0;
    while (il) {
      tmp = il;
      il = il->next;
      delete tmp;
    }
    while (cl) {
      tmp = cl;
      cl = cl->next;
      delete tmp;
    }
  }
  if (!i || !c || !i->parent)
    return;
  XWindowChanges wc;
  wc.sibling = c->frame;
  wc.stack_mode = Below;
  XConfigureWindow(dpy,i->frame,CWStackMode|CWSibling,&wc);
  int istacking = i->GetStacking();
  if (i->mapped && i->parent->LastRaised[istacking] == this) {
    i->GrabButtons();
    i->parent->LastRaised[istacking] = 0;
  }

}

void Client::GrabButtons(bool first) {
  bool MustBeGrabbed = false;
  if ( ((flags & CF_FOCUSONCLICK) && ct->Focus != this)) {
    MustBeGrabbed = true;
  } else {
    if (flags & CF_RAISEONCLICK) {
      for (Client *c = this; c->parent; c = c->parent)
        if (c->parent->LastRaised[c->GetStacking()] != c) {
          MustBeGrabbed = true;
          break;
        }
    }
  }

  if (MustBeGrabbed) {
    if (!(flags & CF_GRABBED) || first) { // must be grabbed
      flags |= CF_GRABBED;// sync, async
      XGrabButton(dpy,AnyButton,AnyModifier,window,True,ButtonPressMask,GrabModeSync,GrabModeAsync,None,None);
    }
  } else {
    if ((flags & CF_GRABBED) || first) { // must be ungrabbed
      flags &= ~CF_GRABBED;
      XUngrabButton(dpy,AnyButton,AnyModifier,window);
      if (flags & (CF_GRABALTCLICK)) {
        XGrabButton(dpy,AnyButton,Mod1Mask,window,false,ButtonPressMask|ButtonReleaseMask|ButtonMotionMask,GrabModeAsync,GrabModeAsync,None,None);
        XGrabButton(dpy,AnyButton,Mod1Mask | ControlMask,window,false,ButtonPressMask|ButtonReleaseMask|Button1MotionMask|Button3MotionMask,GrabModeAsync,GrabModeAsync,None,None);
        XGrabButton(dpy,AnyButton,Mod1Mask | ShiftMask,window,false,ButtonPressMask|ButtonReleaseMask|ButtonMotionMask,GrabModeAsync,GrabModeAsync,None,None);
      }
    }
  }

}

void Client::GiveFocus() {
  if (!parent || parent->focus != this)
    return;
  ct->Focus = 0;
  parent->focus = 0;
  if (!parent->name) {
    delete [] parent->wmname;
    parent->wmname = new char[8];
    strcpy(parent->wmname,"Desktop");
    parent->UpdateName(false);
  }
  if (ct->Focus == this)
    ct->Focus = 0;
}

void Client::GetFocus() {
  if (!parent) {
    ct->CurrentDesktop = (Desktop *)this;
    UEH->SetCurrent(this,0);
    ct->Focus = this;
    return;
  }
  if (trans) {
    Client *lasttrans = 0;
    for (Client *i = this;i;i=i->trans)
      if (i->mapped)
        lasttrans=i;
    if (lasttrans && lasttrans != this) {
      lasttrans->GetFocus();
      return;
    }
  }
  if (!visible)
    return;

  bool fullscreen = false;
  Client *oldfocus = ct->Focus;
  XSetInputFocus(dpy, window, RevertToPointerRoot, CurrentTime);
  XInstallColormap(dpy, cmap);

  ct->Focus = this;
  GrabButtons();
  if (oldfocus)
    oldfocus->GrabButtons();

  Desktop *d,*cd;
  if (target) {
    d = target;
  } else {
    d = DesktopAbove();
    if (d->target)
      d = d->target;
  }

  cd=ct->CurrentDesktop;
  ct->CurrentDesktop = d;
  Client *c = 0;
  if (parent) {
    c = parent->focus;
    parent->focus = this;
  }

  if (oldfocus != this)
    UpdateName(true);
  // ReDraw old and new focus
  Clear();
  if (c)
    c->Clear();

// Their parents and the old and the new CurrentDesktop must be ReDrawed
// i.e. c->parent, lasttrans->parent, cd, d

  if (parent)
    parent->Clear();
  if (cd)
    cd->Clear();
  if (d)
    d->Clear();
  if (c && c->parent)
    c->parent->Clear();


  UEH->SetCurrent(this,0);
}

Client *Client::Focus() {
  return this;
}


void Client::Map() {
  if (visible || !parent)
    return;
  if (!parent->visible)
    parent->Map();
  visible = true;
  if (icon) {
    icon->parent->RemoveIcon(icon);
    icon = 0;
  }
  if (!mapped) {
    XMapWindow(dpy, window);
    XMapWindow(dpy, frame);
    mapped = true;
    Raise(0);
    SetWMState();
    if (flags & CF_HASTBENTRY)
      parent->UpdateTB();
  }
  man->WantFocus = this;
}

void Client::Unmap(bool event) {
  if (ignoreunmap && event) {
    --ignoreunmap;
    return;
  }
  if (!mapped || !parent)
    return;
  mapped = false;
  visible = false;
  XUnmapWindow(dpy, window);
  XUnmapWindow(dpy, frame);
  if (ct->MaxWindow == this) {
    Maximize(MAX_FULLSCREEN);
  }
  SetWMState();
  GiveFocus();
  if (flags & CF_HASTBENTRY)
    parent->UpdateTB();
  parent->AutoClose();
}


void Client::Hide() {
  if (!parent)
    return;
  if (flags & CF_NOMINIMIZE)
    return;
  Unmap(false);
  icon = parent->AddIcon(new Icon(parent,this,0));
  icon->Init();
  SetWMState();
}


/* Attempt to follow the ICCCM by explicity specifying 32 bits for
 * this property. Does this goof up on 64 bit systems? */

void Client::SetWMState() {
  if (ISDESKTOP(this))
    return;
  CARD32 data[2];

  data[0] = visible ? NormalState : ((icon || mapped) ? IconicState : WithdrawnState);
  data[1] = icon ? icon->window : None;

  XChangeProperty(dpy, window, wm_state, wm_state,
      32, PropModeReplace, (unsigned char *)data, 2);

}

/* If we can't find a WM_STATE we're going to have to assume
 * Withdrawn. This is not exactly optimal, since we can't really
 * distinguish between the case where no WM has run yet and when the
 * state was explicitly removed (Clients are allowed to either set the
 * atom to Withdrawn or just remove it... yuck.) */

long Client::GetWMState() {
  if (ISDESKTOP(this))
    return NormalState;
  Atom real_type;
  int real_format;
  unsigned long items_read, items_left;
  long *data, state = WithdrawnState;

  if (XGetWindowProperty(dpy, window, wm_state, 0L, 2L, False,
          wm_state, &real_type, &real_format, &items_read, &items_left,
          (unsigned char **) &data) == Success && items_read) {
      state = *data;
      XFree(data);
  }
  return state;
}

// This will need to be called whenever we update our Client stuff.
// it's a bit difficult:
// y_root is the StaticGravity position of the window
// the origin of y is the NorthWestGravity origin of the desktop

void Client::SendConfig() {
  XConfigureEvent ce;
  bool Max = ct->MaxWindow == this;
  if (Max) {
    x_root = VX;
    y_root = VY;
  } else {
    if (parent) {
      x_root = parent->x_root + x + Sc->BW;
      y_root = parent->y_root + y + Sc->BW;
    } else {
      x_root = x + Sc->BW;
      y_root = y + Sc->BW; // + THeight ?
    }
  }
  if (!window)
    return;
  ce.type = ConfigureNotify;
  ce.event = window;
  ce.window = window;

  ce.x = x_root;
  ce.y = y_root;
  ce.width = Max ? VWidth : width;
  ce.height = Max ? VHeight : height;
  ce.border_width = 0;
  ce.above = None;
  ce.override_redirect = 0;

  XSendEvent(dpy, window, False, StructureNotifyMask, (XEvent *)&ce);
}


void Client::Configure(XConfigureRequestEvent e) {
  if (ct->MaxWindow == this) {
    ChangeSize();
    return;
  }
  XWindowChanges wc;
  if (e.value_mask & CWX) {
    x = e.x - (parent ? parent->x_root : 0) - Sc->BW;
  }

  if (e.value_mask & CWY)
    y = e.y + (parent ? -parent->y_root : 0) - Sc->BW;
  if (e.value_mask & CWWidth) {
    width = e.width;
  }
  if (e.value_mask & CWHeight) {
    height = e.height;
  }
  if (e.value_mask & (CWX | CWY | CWHeight | CWWidth)) {
    if (e.value_mask & CWY)
      Gravitate(1);
    if (e.value_mask & (CWHeight | CWWidth)) {
      ChangeSize();
    } else {
//      if (!(parent->flags & DF_AUTORESIZE)) // i think we should ignore this
      ChangePos(true);
    }
  }

// Applications are so stupid, they don't know what they do if they for example
// put a window on top of the stack
// if applications specify shit like BottomIf or Opposite, it's their own fault that this doesn't work...
// i currently pass a configurerequest with a sibling hoping that apps know what they're doing
  if (e.value_mask & CWStackMode)
    if (e.value_mask & ~CWSibling) {
      e.value_mask &= ~CWSibling & ~CWStackMode;
      if (e.detail == Below)
	Lower(0); else
	Raise(R_INDIRECT);
    } else {
      wc.sibling = e.above;
      wc.stack_mode = e.detail;
      int stacking = GetStacking();
      if (parent && parent->LastRaised[stacking] == this)
	parent->LastRaised[stacking] = 0;
    }
}


/* All toolkits suck. GTK sucks in particular, because if you even
 * -look- at it funny, it will put a PSize hint on your window, and
 * then gleefully leave it at the default setting of 200x200 until you
 * change it. So PSize is pretty useless for us these days.
 *
 * After we get the mouse position, we check for a reasonable position
 * hint, and set x/y relative to the mouse if there is none. To
 * account for window gravity when using the mouse, we add theight in
 * the calculation and then degravitate. Don't think about it too
 * hard, or your head will explode. */

void Client::InitPosition(){
  int xmax = parent->width;
  int ymax = parent->height;
  int mouse_x, mouse_y;

  if (size->flags & (/*PSize|*/USSize)) { // How should the program know where it's placed best?
    width = size->width;
    height = size->height;
  }

  /* make sure it's big enough to click at */
  if (width < 2 * THeight)
    width = 2 * THeight;
  if (height < THeight)
    height = THeight;

  GetMousePosition(parent ? parent->window : root,mouse_x, mouse_y);

  /* position the frame */
  if (width == xmax && height == ymax && !(parent->flags & (DF_TILE | DF_AUTORESIZE))) {
    x = 0;
    y = - THeight;
    MaxX = - Sc->BW;
    MaxY = - Sc->BW;
    MaxW = width;
    MaxH = height;
  } else {
    if (0 <= mouse_x && mouse_x <= xmax && 0 <= mouse_y && mouse_y <= ymax) {
      if (width > xmax) {
        x = 0;
      } else {
        x = (mouse_x * (xmax - width) / xmax);
        if (x + width > xmax)
          x = xmax - width;
      }
      if (height + THeight > ymax) {
        y = 0;
      } else {
        y = (mouse_y * (ymax - height - THeight) / ymax);
        if (y + height > ymax)
          y = ymax - height;
      }
    } else {
// perhaps we should introduce a policy that maximizes the totally used space
      x=0;y=0;
    }
  }
  /* now align the client properly within it */

  y += THeight;
  Gravitate(-1);

  /* hints will override mouse position */
  if (size->flags & (/*PPosition|*/USPosition)) {
      if (size->x > 0 && size->x < xmax)
          x = size->x;
      if (size->y > 0 && size->y < ymax)
          y = size->y;
  }
  x-=Sc->BW;y-=Sc->BW;

}

void Client::Gravitate(int multiplier) {
  int dy = 0;
  int gravity;
  if (flags & CF_FORCESTATIC)
    gravity = StaticGravity; else
    gravity = (size && (size->flags & PWinGravity)) ?
               size->win_gravity : NorthWestGravity;
  switch (gravity) {
    case NorthWestGravity:
    case NorthEastGravity:
    case NorthGravity:
      dy = THeight;
      break;
    case EastGravity:
    case WestGravity:
    case CenterGravity:
      dy = THeight/2;
      break;
  }

  y += multiplier * dy;
  y_root += multiplier *dy;
}


void Client::Reparent() {
  XSetWindowAttributes pattr;

  pattr.override_redirect = True;
  pattr.background_pixel = Sc->colors[C_BG].pixel;
  pattr.border_pixel = Sc->colors[C_BD].pixel;
  pattr.event_mask = ChildMask|ButtonMask|ExposureMask|EnterWindowMask|LeaveWindowMask;
  int X = x;
  int Y = y;
  if (parent) {
    parent->Translate(&X,&Y);
    frame = XCreateWindow(dpy, parent ? parent->window : root,
        X, Y - THeight, width, height + THeight, Sc->BW,
        DefaultDepth(dpy, screen), CopyFromParent, DefaultVisual(dpy, screen),
        CWOverrideRedirect|CWBackPixel|CWBorderPixel|CWEventMask, &pattr);
    pattr.event_mask = ButtonMask|ExposureMask|LeaveWindowMask;
  } else {
    frame = root;
  }
  title = XCreateWindow(dpy, parent ? frame : root,
      parent ? 0 : Sc->BW, parent ? 0 : Sc->BW, width, THeight ? THeight : 1, 0,
      DefaultDepth(dpy, screen), CopyFromParent, DefaultVisual(dpy, screen),
      CWOverrideRedirect|CWEventMask, &pattr);
#ifdef SHAPE
  if (shape && window) {
    XShapeSelectInput(dpy, window, ShapeNotifyMask);
    SetShape();
  }
#endif

// only for non-treewm objects i.e. for non-desktops
  if (!ISDESKTOP(this)) {
    XAddToSaveSet(dpy, window);
    XSelectInput(dpy, window, /*ColormapChangeMask|*/PropertyChangeMask|LeaveWindowMask);
  }
  if (window) {
    XSetWindowBorderWidth(dpy, window, 0);
    XResizeWindow(dpy, window, width, height);
    XReparentWindow(dpy, window, frame, 0, THeight);
    if (mapped)
      ++ignoreunmap;
  }
  SendConfig();
}

void Client::SendEvent(XAnyEvent &e) {
  e.window = window;
  XSendEvent(dpy,window,false,
      (e.type == KeyPress) ? KeyPressMask : KeyReleaseMask,(XEvent *)&e);
}

void Client::SendWMDelete(int mode) {
  if (/*soft &&*/(flags & CF_NOCLOSE) || mode == DEL_RELEASE)
    return;
  int i, n, found=0;
  Atom *protocols;
  if (mode != DEL_FORCE) {
    if (XGetWMProtocols(dpy, window, &protocols, &n)) {
      for (i=0; i<n; ++i)
        if (protocols[i] == wm_delete)
          ++found;
      XFree(protocols);
    }

    if (found) {
      XEvent e;

      e.type = ClientMessage;
      e.xclient.window = window;
      e.xclient.message_type = wm_protos;
      e.xclient.format = 32;
      e.xclient.data.l[0] = wm_delete;
      e.xclient.data.l[1] = CurrentTime;

      XSendEvent(dpy, window, False, NoEventMask, &e);
    }
    return;
  }
//  User should notice that he is destroying a client
  XKillClient(dpy, window);
}

void Client::RequestDesktop(bool) {
  if (!parent)
    return;
  Gravitate(-1);
  parent->GiveAway(this,false);
  Desktop *d = new Desktop(parent);
  d->x = x;
  d->y = y;
  d->width = width;
  d->height = height;
  d = parent->AddDesktop(d,this);
  if (!d)
    return;
  d->autoclose = true;
  MaxX = -Sc->BW;
  MaxY = -Sc->BW;
  d->Take(this,MaxX,MaxY);
  Maximize(MAX_FULL);
}

#include <unistd.h>
void Client::ChangeSize() {
  if (!parent) 
    return;
  if (ct->MaxWindow == this) {
    SendConfig();
    return;
  }
  if (flags & CF_HASTITLE)
    XResizeWindow(dpy, title, width, THeight);
  int X = x;
  int Y = y-THeight;
  parent->Translate(&X,&Y);
  XMoveResizeWindow(dpy, frame, X, Y, width, height + THeight);
  XMoveResizeWindow(dpy, window, 0, THeight, width, height);
  if (parent) {
    parent->Tile();
    parent->AutoResize();
  }
  SendConfig();
#ifdef SHAPE
  if (shape)
    SetShape();
#endif
  UpdateTB();
}

void Client::ChangePos(bool autoresize) {
  if (!parent)
    return;
  if (ct->MaxWindow == this) {
    SendConfig();
    return;
  }
  int X = x;
  int Y = y-THeight;
  parent->Translate(&X,&Y);
  XMoveWindow(dpy,frame,X,Y);
  SendConfig();
  if (!(parent->flags & DF_TILE) && autoresize)
    parent->AutoResize();
}

void Client::Maximize(int type) {
  if (type == MAX_FULLSCREEN || ct->MaxWindow == this) {
    if  (!mapped && ct->MaxWindow != this)
      return;
    if (type != MAX_FULLSCREEN) 
      return;
    Client *oldmax = ct->MaxWindow;
    
    if (ct->MaxWindow == this) {
      ++ignoreunmap;
#ifdef VIDMODE
      if (DHeight != VHeight || DWidth != VHeight) {
//	XUngrabPointer(dpy,CurrentTime);
//	XUngrabButton(dpy,Button3,AnyModifier,window);
      }
#endif
      XReparentWindow(dpy,window,frame,0,0);
      ct->MaxWindow = 0;
      ChangeSize();
    } else {
      if (parent) {
#ifdef VIDMODE
	XF86VidModeModeLine ml;
	int dotclock;
	XF86VidModeGetModeLine(dpy,screen,&dotclock,&ml);
	VWidth = ml.hdisplay;
	VHeight = ml.vdisplay;
	if (DHeight != VHeight || DWidth != VHeight)
	  XF86VidModeGetViewPort(dpy,screen,&VX,&VY);
#endif
	XMoveResizeWindow(dpy,window,0,0, VWidth, VHeight);
	++ignoreunmap;
	XReparentWindow(dpy,window,root,VX,VY);
	XRaiseWindow(dpy,window);
	if (UEH->dialog)
	  XRaiseWindow(dpy,UEH->dialog->window);
	XSetInputFocus(dpy,window, RevertToPointerRoot, CurrentTime);
#ifdef VIDMODE
	if (DHeight != VHeight || DWidth != VHeight) {
  //      XGrabButton(dpy,AnyButton,AnyModifier,window,True,MouseMask,GrabModeSync,GrabModeAsync,None,None);
  //	XGrabPointer(dpy,window,True,0,GrabModeAsync,GrabModeAsync,window,None,CurrentTime);
  //	XGrabButton(dpy,Button3,AnyModifier,root,true,0,GrabModeAsync,GrabModeAsync,window,None);
	}
#endif
      }
      if (oldmax)
	oldmax->Maximize(MAX_FULLSCREEN);
      if (parent)
	ct->MaxWindow = this;
	  
      SendConfig();
    }
    return;
  }
  if (!parent)
    return;
  if (type == MAX_UNMAX && !MaxW && !MaxH)
    return;
  if (type == MAX_REMAX) {
    if (parent->flags & DF_AUTORESIZE)
      return;
    height = parent->height - Sc->BW - y;
    width =  parent->width;

  } else {
    int MaxOld;
    if (MaxH) {
      if (y == -Sc->BW) 
	MaxOld = MaxW ? MAX_FULL : MAX_VERTFULL; else
	MaxOld = MaxW ? MAX_HALF : MAX_VERTHALF;
    } else {
      MaxOld = MaxW ? MAX_HORI : 0;
    }
    if (MaxW) {
      x = MaxX; width = MaxW;
      MaxX = 0; MaxW = 0;
    }
    if (MaxH) {
      y = MaxY; height = MaxH;
      MaxY = 0; MaxH = 0;
    }
    if (type != MAX_UNMAX && type != MaxOld) {
      if (type != MAX_VERTHALF && type != MAX_VERTFULL) {
	MaxX = x; MaxW = width;
	x = -Sc->BW;
	width=parent->width;
      }
      if (type != MAX_HORI) {
        MaxY = y; MaxH = height;
	y = -Sc->BW + ((type==MAX_FULL || type==MAX_VERTFULL) ? 0 : THeight);
	height=parent->height - Sc->BW - y;
      }
    }
  }

  RecalcSweep(S_RIGHT | S_BOTTOM);
  ChangeSize();

}


void Client::Shade() {
  if (!parent)
    return;
  int X = x;
  int Y = y;
  parent->Translate(&X,&Y);
  XMoveResizeWindow(dpy,frame,X,Y-THeight,width,THeight + ((flags & CF_SHADED) ? height : -1));
  flags ^= CF_SHADED;
  mask |= CF_SHADED;
}


/* After pulling my hair out trying to find some way to tell if a
 * window is still valid, I've decided to instead carefully ignore any
 * errors raised by this function. We know that the X calls are, and
 * we know the only reason why they could fail -- a window has removed
 * itself completely before the Unmap and Destroy events get through
 * the queue to us. It's not absolutely perfect, but it works.

 * Ick. Argh. You didn't see this function. */

int ignore_xerror(Display *dpy, XErrorEvent *e)
{
  return 0;
}

Client::~Client() {
  if (icon) {
    icon->parent->RemoveIcon(icon);
    icon = 0;
  }
}

void Client::Remove(int mode) {
  XGrabServer(dpy);
  XSetErrorHandler(ignore_xerror);
  XUngrabButton(dpy,AnyButton,AnyModifier,window);
#ifdef DEBUG
  err("removing %s, %d: %d left", name ? name : wmname, mode, XPending(dpy));
#endif
  Gravitate(-1);
  if (parent) {
    ct->windows.erase(window);
  }
  ct->windows.erase(frame);
  ct->windows.erase(title);
  if (mode != R_RESTART)
    if (mode == R_WITHDRAW) {
      visible = false;
      mapped = false;
      SetWMState();
    } else { // R_REMAP
      if (icon)
        Map();
      if (parent)
        if (mapped/* || icon*/)
          XMapWindow(dpy, window); else
          XUnmapWindow(dpy, window);
    }
  if (!ISDESKTOP(this)) {
    XReparentWindow(dpy, window, root, x_root - Sc->BW, y_root - Sc->BW);
    XRemoveFromSaveSet(dpy, window);
    if (mapped)
      ++ignoreunmap;
  } else {
    if (parent)
      XDestroyWindow(dpy, window);
  }

  int stacking = GetStacking();
  if (parent && parent->LastRaised[stacking] == this)
    parent->LastRaised[stacking] = 0;

  if (parent)
    XSetWindowBorderWidth(dpy, window, 1);

  if (title)
    XDestroyWindow(dpy, title);
  if (frame)
    XDestroyWindow(dpy, frame);

  if (size)
    XFree(size);
  if (name) {
    free(name);
    name = 0;
  }

  if (trans)
    trans->transfor=transfor;

  if (transfor) {
    transfor->trans=trans;
  }

  if (mode == R_WITHDRAW)
    ct->RemoveClientReferences(this);

  XSync(dpy, False);
  XSetErrorHandler(handle_xerror);
  XUngrabServer(dpy);
}

void Client::ToggleHasTitle() {
  Gravitate(-1);
  flags ^= CF_HASTITLE;
  if (flags & CF_HASTITLE) {
    XFontSetExtents *e;
    e = XExtentsOfFontSet(Sc->fonts[FO_STD]);
    THeight= e->max_logical_extent.height*4/5 + e->max_logical_extent.height/5 + 2*SPACE;
    Desktop *d = ISDESKTOP(this);
    if (d)
      d->UpdateTB();
    if (!parent)
      height -= THeight;
    XMapWindow(dpy,title);
  } else {
    if (!parent)
      height += THeight;
    THeight = 0;
    XUnmapWindow(dpy,title);
  }
  Gravitate(1);
  ChangeSize();
}


void Client::UpdateTB() {
  TBx = 0;
  TBy = width;
  Clear();
}

void Client::ChangeName() {
  if (ISDESKTOP(this))
    return;
  if (name)
    free(name);
  char *windowname = 0;
  FetchName(window, &windowname);
  if (!windowname || !windowname[0]) {
    name = 0;
  } else {
    int length = strlen(windowname);
    if (length > MAXTITLE)
      length = MAXTITLE;
    windowname[length]=0;
    name = strdup(windowname);
  }
  if (windowname)
    XFree(windowname);

}

/* UpdatName() is called whenever a window gets the focus or changes its name
 */
void Client::UpdateName(bool setFocus) {
  Clear();
  if (!parent)
    return;
  if (setFocus) {
    parent->focus = this;
  } else {
    if (parent->focus != this) {
      parent->Clear();
      return;
    }
  }
  for (Desktop *ud=parent;;ud = ud->parent) {
    ud->Clear();
    if (ud->name)
      return;
    char *fname = ud->focus->name ? ud->focus->name : ud->focus->wmname;
    int n = strlen(fname);
    if (ud->wmname)
      delete [] ud->wmname;
    ud->wmname = new char[n+3];
    ud->wmname[0] = '[';
    ud->wmname[1] = '\0';
    strcat(ud->wmname,fname);
    strcat(ud->wmname,"]");
    if (!ud->parent)
      return;
    if (setFocus) {
      ud->parent->focus = ud;
    } else {
      if (ud->parent->focus != ud) {
        ud->parent->Clear();
        return;
      }
    }
  }
}


void Client::ReDraw() {
  if (!(flags & CF_HASTITLE) || !visible)
    return;
  XDrawLine(dpy, title, Sc->border_gc,
    0, THeight - 1,
    width, THeight - 1);
  ReDrawTBEntry(this);
}

void Client::ReDrawTBEntry(Client *t,bool all) {
  int tx,ty;
  bool NoButtons;
  if (t==this) {
    tx = TBx;
    ty = TBy;
    NoButtons = !(flags & CF_TITLEBARBUTTONS);
  } else {
    tx = t->TBEx;
    ty = t->TBEy;
    NoButtons = !(flags & DF_TASKBARBUTTONS);
  }
  int hl=0;
  if (t == this) {
    if (!t->parent || t->parent->focus==t)
      ++hl;
  } else {
    if (t->parent->focus == t)
      ++hl;
  }
  if (t == UEH->Current)
    hl = 2;
  if (all) {
    XFontSetExtents *e;
    e = XExtentsOfFontSet(Sc->fonts[FO_STD]);
    XFillRectangle(dpy, title, Sc->title_gc[hl],
      tx, 0,
      ty, THeight - 1);
    char *str = t->name ? t->name : t->wmname;
     XmbDrawString(dpy, title, Sc->fonts[FO_STD], Sc->string_gc,
      tx + SPACE, SPACE + e->max_logical_extent.height*4/5,
      str, strlen(str));
    XDrawLine(dpy, title, Sc->border_gc,
      ty, 0,
      ty, THeight);
    if (NoButtons)
      return;
    XDrawLine(dpy, title, Sc->border_gc,
      ty - BUTTONWIDTH, 0,
      ty - BUTTONWIDTH, THeight);
    if (!(t->flags & CF_FIXEDSIZE))
      XDrawLine(dpy, title, Sc->border_gc,
        ty - 2*BUTTONWIDTH, 0,
        ty - 2*BUTTONWIDTH, THeight);
  }

  if (NoButtons)
    return;
  RPixmap *pm;
  int b=0;
  if (UEH->SelClient == this && UEH->SelEntry ==t)
    if ((UEH->Seltype & T_BUTTON) &&
        (UEH->Seltype & T_BUTTON) != T_BAR) {
      if ((UEH->Seltype & T_BUTTON) == T_F2)
        b+=4;
      b += (UEH->Seltype & T_MOUSEBUTTON);
    }
  pm = Sc->button[b>=4 ? b : BU_20];
  if (pm && pm->pm)
    XCopyArea(dpy,pm->pm,title,Sc->border_gc,
              0,0,pm->w,pm->h,ty - (BUTTONWIDTH+pm->w)/2,(THeight-pm->h)/2); else
    XFillRectangle(dpy,title,Sc->title_gc[hl],
                   ty - BUTTONWIDTH+1,0,BUTTONWIDTH-1,THeight-1);

  if (!(t->flags & CF_FIXEDSIZE)) {
    pm = Sc->button[(!b || b>=4) ? BU_10 : b];
    if (pm && pm->pm)
      XCopyArea(dpy,pm->pm,title,Sc->border_gc,
                0,0,pm->w,pm->h,ty - (3*BUTTONWIDTH+pm->w)/2,(THeight-pm->h)/2); else
      XFillRectangle(dpy,title,Sc->title_gc[hl],
                     ty - 2*BUTTONWIDTH+1,0,BUTTONWIDTH-1,THeight-1);
  }

}






void Client::DrawOutline() {
  if (!parent)
    return;
  char buf[32];
  int w, h;
  int X = x, Y = y;
  parent->Translate(&X,&Y);
  XDrawRectangle(dpy, parent ? parent->window : root , Sc->invert_gc,
      X + Sc->BW/2, Y - THeight + Sc->BW/2,
      width + Sc->BW, height + THeight + Sc->BW);
  if (THeight)
    XDrawLine(dpy, parent ? parent->window : root, Sc->invert_gc, X + Sc->BW/2+1, Y + Sc->BW - 1,
      X + width + 3*Sc->BW/2, Y + Sc->BW - 1);


  int basex, basey;

  if (size->flags & PResizeInc) {
    basex = (size->flags & PBaseSize) ? size->base_width :
            (size->flags & PMinSize) ? size->min_width : 0;
    basey = (size->flags & PBaseSize) ? size->base_height :
            (size->flags & PMinSize) ? size->min_height : 0;
    w = (width - basex) / size->width_inc;
    h = (height - basey) / size->height_inc;
  } else {
    w = width;
    h = height;
  }

  Gravitate(-1);
  snprintf(buf, sizeof buf, "%dx%d+%d+%d", w, h, x, y);
  Gravitate(1);
  XmbDrawString(dpy, parent ? parent->window : root, Sc->fonts[FO_STD], Sc->invert_gc,
      X + width - XmbTextEscapement(Sc->fonts[FO_STD], buf, strlen(buf)) - SPACE,
      Y + height - SPACE,
      buf, strlen(buf));

}

void MoveMouse(int key) {
  if (key == XKeysymToKeycode(dpy,XK_Left)) {
    XWarpPointer(dpy,None,None,0,0,0,0,-10,0);
  }
  if (key == XKeysymToKeycode(dpy,XK_Right)) {
    XWarpPointer(dpy,None,None,0,0,0,0,10,0);
  }
  if (key == XKeysymToKeycode(dpy,XK_Up)) {
    XWarpPointer(dpy,None,None,0,0,0,0,0,-10);
  }
  if (key == XKeysymToKeycode(dpy,XK_Down)) {
    XWarpPointer(dpy,None,None,0,0,0,0,0,10);
  }
}

void Client::MoveResize(int mode, bool movemouse,int x1, int y1, bool firstevent) {
  if (!parent)
    return;
  if (ct->MaxWindow == this)
    return;
  if (MaxW || MaxH) {
    Maximize(MAX_UNMAX);
    ReDraw();
  }

  XEvent ev;
  int old_x = x;
  int old_y = y;
  int old_w = width;
  int old_h = height;

  if (!grabp(root, MouseMask, Sc->cursors[CU_MOVE]))
    return;
  XGrabKeyboard(dpy,root,false,GrabModeAsync,GrabModeAsync,CurrentTime);
//  XGrabKey(dpy,AnyKey,AnyModifier,parent->window,false,GrabModeAsync,GrabModeAsync);

  XGrabServer(dpy);

  if (movemouse) {
    if (mode & S_LEFT)
      x1 = x_root; else
      if (mode & S_RIGHT)
        x1 = x_root+width; else
        x1 = x_root+width/2;
    if (mode & S_TOP)
      y1 = y_root - THeight; else
      if (mode & S_BOTTOM)
        y1 = y_root+height; else
        y1 = y_root-THeight/2;
    setmouse(root,x1,y1);
  }
#ifdef DEBUG
    ShowEvent(ev);
#endif

  DrawOutline();
  for (bool alive = true;alive;) {
    XMaskEvent(dpy, MouseMask | KeyPressMask, &ev);
    switch (ev.type) {
      case KeyPress:
        MoveMouse(ev.xkey.keycode);
        if (ev.xkey.keycode == XKeysymToKeycode(dpy,XK_Escape)) {
          alive = false;
          DrawOutline(); /* clear */
          x = old_x; width  = old_w;
          y = old_y; height = old_h;
          DrawOutline();
        } else if (ev.xkey.keycode == XKeysymToKeycode(dpy,XK_space) ||
                   ev.xkey.keycode == XKeysymToKeycode(dpy,XK_Return))
            alive = false;

        break;
      case MotionNotify:
        DrawOutline(); /* clear */
        switch (mode & (S_LEFT|S_RIGHT)) {
          case S_RIGHT:
            width = old_w + ev.xmotion.x - x1;
            break;
          case S_LEFT:
            width = old_w - ev.xmotion.x + x1;
          case 0:
            x = old_x + ev.xmotion.x - x1;
            break;
        }
        switch (mode & (S_TOP|S_BOTTOM)) {
          case S_BOTTOM:
            height = old_h + ev.xmotion.y - y1;
            break;
          case S_TOP:
            height = old_h - ev.xmotion.y + y1;
          case 0:
            y = old_y + ev.xmotion.y - y1;
            break;
        }
        Snap(mode);
        if (mode)
          RecalcSweep(mode);
        DrawOutline();
        break;
      case ButtonPress:
        firstevent = false;
        break;
      case ButtonRelease:
        if (firstevent)
          firstevent = false; else
          alive = false;
        break;
    }
  }

  DrawOutline(); /* clear */
  XUngrabServer(dpy);
  XUngrabPointer(dpy, CurrentTime);
  if (!UEH->dialog)
    XUngrabKeyboard(dpy,CurrentTime);

  if (mode)
    ChangeSize(); else
    ChangePos(true);
}

#define MAX(x,y) (x)>(y) ? (x) : (y)


void Client::Snap(int mode) {
  if (!parent || !(flags & CF_SNAP))
    return;
  int x1[2],x2[2],y1[3],y2[3];
  int minx=snap+1,miny=snap+1,dist;
  int dminus = - (Sc->BW / 2);
  int dplus = (Sc->BW + 1) / 2;
  x1[0] = x + dminus;
  x1[1] = x + width + dplus;
  y1[0] = y - THeight + dminus;
  y1[1] = y + dminus;
  y1[2] = y + height + dplus;
  for (Client *c=parent->firstchild;c;c=c->next) {
    if (!c->mapped)
      continue;
    dminus = - (c->Sc->BW / 2);
    dplus = (c->Sc->BW + 1) / 2;
    if (c!=this) {
      x2[0] = c->x + dminus;
      x2[1] = c->x + c->width + dplus;
      y2[0] = c->y - c->THeight + dminus;
      y2[1] = c->y + dminus;
      y2[2] = c->y + c->height + dplus;
      if (!(x1[0] <= x2[1]+snap && x2[0] <= x1[1]+snap &&
            y1[0] <= y2[2]+snap && y2[0] <= y1[2]+snap))
        continue;
    } else {
      c = parent; // temporalily look at the parent window
      x2[0] = - Sc->BW + dminus;
      y2[0] = - Sc->BW - c->THeight + dminus;
      y2[1] = - Sc->BW + dminus;
      if (parent->flags & DF_AUTORESIZE) {
        x2[1] = x2[0];
        y2[2] = y2[0];
      } else {
        x2[1] = - Sc->BW + c->width + dplus;
        y2[2] = - Sc->BW + c->height + dplus;
      }
    }

    int min = (mode & S_RIGHT) ? 1 : 0;
    int max = (mode & S_LEFT)  ? 1 : 2;
    for (int i = min; i!=max; ++i)
      for (int j = 0; j!=2; ++j) {
        dist = x2[j] - x1[i];
        if (abs(dist) < abs(minx))
          minx = dist;
      }

    min = (mode & S_BOTTOM) ? 2 : 0;
    max = (mode & S_TOP)    ? 2 : 3;
    for (int i = min; i!=max; ++i)
      for (int j = 0; j!=3; ++j) {
        dist = y2[j] - y1[i];
        if (abs(dist) < abs(miny))
          miny = dist;
      }

    if (c==parent)
      c=this; // We must not forget this
  }

  if (abs(minx) <= snap)
    switch (mode & (S_LEFT | S_RIGHT)) {
      case S_RIGHT:
        width += minx;
        break;
      case S_LEFT:
        width -= minx;
      default:
        x += minx;
    }
  if (abs(miny) <= snap)
    switch (mode & (S_TOP | S_BOTTOM)) {
      case S_BOTTOM:
        height += miny;
        break;
      case S_TOP:
        height -= miny;
      default:
        y += miny;
    }



}


void Client::RecalcSweep(int mode) {
  int basex, basey,dx,dy;

  if (size->flags & PResizeInc) {
    basex = (size->flags & PBaseSize) ? size->base_width :
        (size->flags & PMinSize) ? size->min_width : 0;
    basey = (size->flags & PBaseSize) ? size->base_height :
        (size->flags & PMinSize) ? size->min_height : 0;
    dx = ((width - basex) % size->width_inc);
    dy = ((height - basey) % size->height_inc);
  } else {
    dx = 0; dy = 0;
  }
  if (size->flags & PMinSize) {
    if (width - dx < size->min_width)
      dx = width - size->min_width;
    if (height - dy < size->min_height)
      dy = height - size->min_height;
  } else {
    if (width - dx < 10)
      dx = width - 10;
    if (height - dy < 10)
      dy = height - 10;
  }

  if (size->flags & PMaxSize) {
    if (width > size->max_width)
      dx = width - size->max_width;
    if (height > size->max_height)
      dy = height - size->max_height;
  }


  width -= dx;
  height -= dy;
  if (mode & S_LEFT)
    x += dx;
  if (mode & S_TOP)
    y += dy;



}



#ifdef SHAPE

void Client::MakeHole(bool add) {
  if (!parent)
    return;
  XEvent ev;
  int x1=0,x2=0,y1=0,y2=0;

  if (!grabp(root, MouseMask, Sc->cursors[CU_RESIZE]))
    return;

  for (int up = 1;up;) {
    XMaskEvent(dpy, MouseMask | KeyPressMask, &ev);
    switch (ev.type) {
      case MotionNotify:
        break;
      case ButtonPress:
        x1 = ev.xbutton.x_root;
        y1 = ev.xbutton.y_root;
        break;
      case ButtonRelease:
        x2 = ev.xbutton.x_root;
        y2 = ev.xbutton.y_root;
        --up;
        break;
    }
  }
  XUngrabPointer(dpy, CurrentTime);

  if (x1 > x2) {
    int x = x1;
    x1 = x2;
    x2 = x;
  }
  if (y1 > y2) {
    int y = y1;
    y1 = y2;
    y2 = y;
  }
  if (x1<0 || x2<0)
    return;
  x1 -= x_root;
  if (x1<0)
    x1 = 0;
  x2 -= x_root;
  if (x2>width)
    x2 = width;
  y1 -= y_root;
  if (y1<0)
    y1 = 0;
  y2 -= y_root;
  if (y2>height)
    y2 = height;
  XRectangle temp;
  temp.x = x1;
  temp.y = y1;
  temp.width = x2 - x1;
  temp.height = y2 - y1;
  XShapeCombineRectangles(dpy, frame, ShapeBounding,
      0, THeight, &temp, 1, add ? ShapeSubtract : ShapeUnion, YXBanded);
  if (add)
    flags |= CF_HASBEENSHAPED;

}


void Client::SetShape() {
  if (!parent)
    return;
  int n, order;
  XRectangle temp, *dummy;

  dummy = XShapeGetRectangles(dpy, window, ShapeBounding, &n, &order);
  if (n > 1) {
    XShapeCombineShape(dpy, frame, ShapeBounding,
        0, THeight, window, ShapeBounding, ShapeSet);
    temp.x = -Sc->BW;
    temp.y = -Sc->BW;
    temp.width = width + 2*Sc->BW;
    temp.height = THeight + Sc->BW;
    XShapeCombineRectangles(dpy, frame, ShapeBounding,
        0, 0, &temp, 1, ShapeUnion, YXBanded);
    temp.x = 0;
    temp.y = 0;
    temp.width = width;
    temp.height = THeight - Sc->BW;
    XShapeCombineRectangles(dpy, frame, ShapeClip,
        0, THeight, &temp, 1, ShapeUnion, YXBanded);
    flags |= CF_HASBEENSHAPED;
  } else if (flags & CF_HASBEENSHAPED) {
    /* I can't find a 'remove all shaping' function... */
    temp.x = -Sc->BW;
    temp.y = -Sc->BW;
    temp.width = width + 2*Sc->BW;
    temp.height = height + THeight + 2*Sc->BW;
    XShapeCombineRectangles(dpy, frame, ShapeBounding,
        0, 0, &temp, 1, ShapeSet, YXBanded);
  }
  XFree(dummy);
}
#endif


#ifdef DEBUG

#define SHOW(name) \
    case name: return #name;

const char *Client::ShowGrav() {
    if (!size || !(size->flags & PWinGravity))
        return "no grav (NW)";

    switch (size->win_gravity) {
        SHOW(UnmapGravity)
        SHOW(NorthWestGravity)
        SHOW(NorthGravity)
        SHOW(NorthEastGravity)
        SHOW(WestGravity)
        SHOW(CenterGravity)
        SHOW(EastGravity)
        SHOW(SouthWestGravity)
        SHOW(SouthGravity)
        SHOW(SouthEastGravity)
        SHOW(StaticGravity)
        default: return "unknown grav";
    }
}


const char *Client::ShowState() {
    switch (GetWMState()) {
        SHOW(WithdrawnState)
        SHOW(NormalState)
        SHOW(IconicState)
        default: return "unknown state";
    }
}


void Client::dump() {
  err("%s\n\t%s, %s, mapped %d\n"
      "\tframe %#lx, title %#lx, win %#lx, geom %dx%d+%d+%d",
      name ? name : wmname, ShowState(), ShowGrav(), mapped,
      frame,title, window, width, height, x, y);
}
#endif

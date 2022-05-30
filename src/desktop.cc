/* treewm - an X11 window manager.
 * Copyright (c) 2001-2003 Thomas Jäger <TheHunter2000 at web dot de>
 * This code is released under the terms of the GNU GPL.
 * See the included file LICENSE for details.
 * 
 * changed<05.02.2003> by Rudolf Polzer: Refreshing root window on startup
 *   so background pixmap works again.
 *   Updating toolbar when reparenting multiple windows at once into a new
 *   desktop.
 */

#include "desktop.h"
#include "clienttree.h"
#include "icon.h"
#include "sceme.h"
#include "action.h"
#include "manager.h"
#include "clientinfo.h"
#include "tile.h"
#include <signal.h>
#include <string.h>

unsigned long Desktop::GetRegionMask(int x, int y, Client * &t) {
  x -= x_root;
  y += THeight - y_root;
  if (0 <= y && y <= THeight && 0 <= x && x <= width) {
    if (!t)
      for (t=firstchild;t;t=t->next)
        if (t->mapped && (t->flags & CF_HASTBENTRY) && t->TBEx <= x && x <= t->TBEy)
          break;
    if (!t && TBx <= x && x <= TBy)
      t=this;
    if (t) {
      if (!(flags & ((t==this) ? CF_TITLEBARBUTTONS : DF_TASKBARBUTTONS)))
        return T_BAR;
      return ((x >= (t == this ? TBy : t->TBEy) - ((t->flags & CF_FIXEDSIZE) ? 1 : 2)*BUTTONWIDTH)
                ? (x >= (t == this ? TBy : t->TBEy) - BUTTONWIDTH) ? T_F2 : T_F1 :
                T_BAR);
    }
  }
  return 0;
}

void Desktop::AutoClose() {
  if (!autoclose || !parent)
    return;
  for (Client *i = firstchild;i;i = i->next)
    if (i->mapped || i->icon)
      return;
  SendWMDelete(DEL_NORMAL);
}


Desktop::Desktop(Desktop *p) : Client(p) {
  childmask = 0;
  firstchild = NULL;
  firsticon = NULL;
  PosX = 0;
  PosY = 0;
  for (int i=0; i!=3; ++i)
    LastRaised[i] = 0;
  focus = NULL;
  wmname = new char[8];
  strcpy(wmname,"Desktop");
  if (parent)
    name = 0; else
    name = strdup("Root Desktop");
  VirtualX = parent ? 1 : 3;
  VirtualY = parent ? 1 : 2;
  bgpm = 0;
}

void Desktop::SetClientInfo(ClientInfo *ci) {
  if (!ci)
    return;
  if (ci && (ci->flags & CI_VIRTUALX))
    VirtualX = ci->VirtualX;
  if (ci && (ci->flags & CI_VIRTUALY))
    VirtualY = ci->VirtualY;
  Client::SetClientInfo(ci);
  if (ci->name) {
    if (name)
      free(name);
    name = strdup(ci->name);
  }
  for (ActionList *al = ci->actions;al;al=al->next)
    AddIcon(new Icon(this,0,al->a));
#define NOPIXMAP (RPixmap *)(-1)
  if (ci->bgpm && bgpm != NOPIXMAP)
    bgpm = ci->bgpm;
  if (!parent && (ci->flags & CI_NOBACKGROUND))
    bgpm = NOPIXMAP;

}

bool Desktop::Init() {
  if (!Sc)
    Sc = (Sceme *)rman->GetInfo(SE_SCEME,"");
  focus = 0;
  if (!bgpm)
    bgpm = Sc->bgpm;

  XSetWindowAttributes pattr;
  unsigned long Mask = 0;
  if (bgpm) {
    if (bgpm != NOPIXMAP) {
      pattr.background_pixmap = bgpm->GetPixmap();
      Mask |= CWBackPixmap;
    }
  } else {
    pattr.background_pixel = Sc->colors[C_BG].pixel;
    Mask |= CWBackPixel;
  }
  if (parent) {
    Mask |= (CWOverrideRedirect|CWEventMask);
    pattr.override_redirect = true;
    pattr.event_mask = ChildMask|ButtonMask|ExposureMask|EnterWindowMask|LeaveWindowMask|KeyPressMask|KeyReleaseMask;
    int X,Y;
    GetMousePosition(parent->window,X,Y);
    window = XCreateWindow(dpy, root, X,Y,300,300,0,
        DefaultDepth(dpy, screen), CopyFromParent, DefaultVisual(dpy, screen),
        Mask, &pattr);
    XMapWindow(dpy,window); // remove this!
  } else {
    window = 0;
    XChangeWindowAttributes(dpy,root,Mask,&pattr);
    XClearWindow(dpy,root);
  }
  if (!Client::Init())
    return false;
  if (window) {
    XMapWindow(dpy,window);
    XMapWindow(dpy,frame);
  }
  man->WantFocus = this;
  if (!parent)
    window = root;

  pattr.override_redirect = true;
  for (int i = 0; i!=3; ++i) {
    StackRef[i] = XCreateWindow(dpy, window,
                  0,0,1,1, 0,
                  DefaultDepth(dpy, screen), CopyFromParent, DefaultVisual(dpy, screen),
                  CWOverrideRedirect, &pattr);
    XRaiseWindow(dpy,StackRef[i]);
  }
    // RP: Fix for "no-refresh" bug
    XClearWindow(dpy,window);
  XRaiseWindow(dpy,title);

  autoclose = false;
  for (Icon *i = firsticon; i; i=i->next)
    i->Init();

  flags |= DF_ICONSRAISED;
  if (!parent)
    Raise(0);

  for (Client *c = firstchild;c;c=c->next)
    c->Init();

  for (int i=0;i!=4;++i)
    if (ct->RootBorder[i])
      XRaiseWindow(dpy,ct->RootBorder[i]);

  return true;
}


Client *Desktop::AddChild(Client *c) {
// Check if there's a default ClientInfo
  if (firstchild) {
    Client *cc;
    for (cc=firstchild;cc->next;cc=cc->next);
    cc->next = c;
  } else {
    firstchild = c;
  }
  c->next = NULL;
  if (!c->Init()) {
    RemoveChild(c,R_WITHDRAW);
    return 0;
  }
  if (c->flags & CF_HASTBENTRY)
    UpdateTB();
  if (ct->Focus == 0 || ct->Focus == this || ct->Focus == focus)
    man->WantFocus = c;
  Tile();
  AutoResize();

  return c;
}

void Desktop::UpdateFlags(int act,unsigned long flag) {
  if (!act)
    return;
  switch (act) {
    case FLAG_CREMOVE:
      childmask &= ~flag;
      break;
    case FLAG_CSET:
      childflags |= flag;
      childmask |= flag;
      break;
    case FLAG_CUNSET:
      childflags &= ~flag;
      childmask |= flag;
      break;
    case FLAG_CTOGGLE:
      childflags &= childmask;
      childflags |= ~childmask & flags;
      childflags ^= flag;
      childmask |= flag;
      break;
    default:
      Client::UpdateFlags(act,flag);
  }
  for (Client *c = firstchild; c; c=c->next)
    c->UpdateFlags(FLAG_UPDATE,0);
  if (flag & flags & DF_AUTORESIZE)
    AutoResize();
}

Desktop *Desktop::AddDesktop(Desktop* c,Client *copyopts) {
  if (copyopts) {
    c->flags = copyopts->flags;
    c->mask = copyopts->mask;
    c->parentmask = copyopts->parentmask;
  }

  if (firstchild) {
    Client *cc;
    for (cc=firstchild;cc->next;cc=cc->next);
    cc->next = c;
  } else {
    firstchild = c;
  }
  c->next = NULL;
  if (!c->Init()) {
    RemoveChild(c,R_WITHDRAW);
    return 0;
  }
  if (c->flags & CF_HASTBENTRY)
    UpdateTB();
  if (!focus)
    man->WantFocus = c;
  return c;
}


Desktop *Desktop::DesktopAbove() {
  return this;
}


Client* Desktop::FindPointerClient() {
  Window w, dummyw;
  int d1,d2,root_x,root_y;
  unsigned int d3;
  Client *tb = 0;
  if (XQueryPointer(dpy,window,&dummyw,&w,&root_x,&root_y,&d1,&d2,&d3)) {
    for (Client *c = firstchild; c; c=c->next) {
      if (w==c->frame)
        return c->FindPointerClient();
    }
  }
  GetRegionMask(root_x-x_root,root_y-y_root+THeight,tb);

  return tb ? tb : this;
}

bool Desktop::GetWindowList(MenuItem *m,int &n,bool mouse,int mx,int my,int ind,char *key) {
  if (!Client::GetWindowList(m,n,mouse,mx,my,ind,key))
    return false;
  if (!mapped)
    return true;
  ++ind;
  for (Client *i=firstchild;i;i=i->next)
    i->GetWindowList(m,n,mouse,mx,my,ind,key);
  return true;
}


void Desktop::RemoveClientReferences(Client *c) {
  Client::RemoveClientReferences(c);
  for (int i = 0; i!=3; ++i)
    if (c == LastRaised[i])
      LastRaised[i] = 0;
  for (Client *cc=firstchild;cc;cc=cc->next) {
    cc->RemoveClientReferences(c);
  }
  for (Icon *i = firsticon; i; i=i->next)
    if (i->client == c) {
      RemoveIcon(i);
      i = firsticon; // I first forgot this
    }

  
}


void Desktop::RemoveChild(Client *c, int mode) {
  if (ct->MaxWindow == c)
    c->Maximize(MAX_FULLSCREEN);
  c->GiveFocus();
  c->Remove(mode);
  if (c==firstchild)
    firstchild = c->next; else
    for (Client *cc=firstchild;cc->next;cc=cc->next)
      if (cc->next==c) {
        cc->next=c->next;
        break;
      }
  if (c->flags & CF_HASTBENTRY)
    UpdateTB();
  delete c;
  Tile();
  AutoResize();
  AutoClose();
}

void Desktop::GiveAway(Client *c, bool autoclose) {
  c->flags &= ~CF_ICONPOSKNOWN; // New Desktop may be smaller
  c->GiveFocus();

  if (c==firstchild)
    firstchild = c->next; else
    for (Client *cc=firstchild;cc->next;cc=cc->next)
      if (cc->next==c) {
        cc->next=c->next;
        break;
      }
  if (!autoclose)
    return;
  if (c->mapped && (c->flags & CF_HASTBENTRY))
    UpdateTB();
  Tile();
  AutoResize();
  AutoClose();

}

void Desktop::SendEvent(XAnyEvent &e) {
  for (Client *i= firstchild;i;i=i->next)
    i->SendEvent(e);
}

void Desktop::SendWMDelete(int mode) {
// Show a dialog box here...
  if (!parent) {
    if (mode != DEL_RELEASE)
      man->alive = false;
    return;
  }
  while (firsticon) {
    if (firsticon->client)
      firsticon->client->Map(); else
      RemoveIcon(firsticon);
  }
  if (firstchild) {
    Client *cc; // if c is deleted we can't access c->next any more
    for (Client *c = firstchild; c; c=cc) {
      cc = c->next;
      if (c) {
        if ((c->mapped || c->icon) && mode != DEL_RELEASE) {
          c->SendWMDelete(mode);
        } else {
          GiveAway(c,false);
          parent->Take(c,c->x + (x_root - parent->x_root),c->y + (y_root - parent->y_root));
        }
      }
    }
  }
  autoclose = true;
  if (!firstchild)
    man->AddDeleteClient(this);
}


void Desktop::Remove(int mode) {
  if (mode != R_WITHDRAW) {
    Goto(-PosX,-PosY,GOTO_RELATIVE);
    for (Client *c=firstchild; c; c=c->next)
      c->Remove(mode);
  } else {
    for (Client *c=firstchild; c; c=firstchild) {
      // That bug was hard to find, the destructor accesses this desktop in
      // RemoveClientReferences
      firstchild = c->next;
      c->Remove(mode);
      delete c;
    }
  }

  for (Icon *i = firsticon;i;i=i->next)
    i->Remove();

  Client::Remove(mode);
  if (wmname)
    delete [] wmname;
  wmname = 0;

  if (bgpm && bgpm != NOPIXMAP)
    bgpm->FreePixmap();

  x += Sc->BW;
  y += Sc->BW + THeight;
}

void Desktop::ChangeSize() {
  Client::ChangeSize();
  if (!parent)
    return;
  for (Client *c=firstchild;c;c=c->next)
    if (c->MaxW || c->MaxH)
      c->Maximize(MAX_REMAX);
  Tile();
}

void Desktop::AutoResize() {
  if (!parent || !(flags & DF_AUTORESIZE))
    return;
  int X = width;
  int Y = height;
  int W = 0;
  int H = 0;
  for (Client *c=firstchild; c; c=c->next) {
    if (!c->mapped)
      continue;
    if (c->x + c->width > W)
      W = c->x +c->width;
    if (c->y + c->height > H)
      H = c->y +c->height;
    if (c->x+c->Sc->BW < X)
      X = c->x+c->Sc->BW;
    if (c->y+c->Sc->BW-THeight < Y)
      Y = c->y+c->Sc->BW-THeight;
  }
  if (!W || !H)
    return;
  width = W + Sc->BW - X;
  height = H + Sc->BW - Y;
  x += X;
  y += Y;
  ChangeSize();
  if (X || Y)
    for (Client *c=firstchild; c; c=c->next) {
      c->x -= X;
      c->y -= Y;
      c->ChangePos(false);
    }
}
// Translates coordinates when children want to move their windows

void Desktop::Translate(int *X,int *Y) {
  if (parent)
    return;
  if (X)
    *X += Sc->BW;
  if (Y)
    *Y += THeight + Sc->BW;
}

void Desktop::ReDraw() {
  if (!(flags & CF_HASTITLE) || !visible)
    return;
  XDrawLine(dpy, title, Sc->border_gc,
    0, THeight - 1,
    width, THeight - 1);
  for (Client *t=firstchild;t;t=t->next)
    if (t->mapped && (t->flags & CF_HASTBENTRY))
      ReDrawTBEntry(t);
  ReDrawTBEntry(this);
}


void Desktop::UpdateTB() {
  if (!(flags & CF_HASTITLE))
    return;
  int i=0;
  int TBNum=1;
  for (Client *t=firstchild;t;t=t->next)
    if (t->mapped && (t->flags & CF_HASTBENTRY))
      ++TBNum;
  for (Client *t=firstchild;t;t=t->next)
    if (t->mapped && (t->flags & CF_HASTBENTRY)) {
      t->TBEx=i*width/TBNum;
      ++i;
      t->TBEy=i*width/TBNum - 1;
    }
  TBx=(TBNum-1)*width/TBNum;
  TBy=width;
  Clear();

}

void Desktop::Raise(int mode) {
  if (!(mode & R_INDIRECT))
    if (parent) {
      ShowIcons(parent->LastRaised[GetStacking()] == this);
    } else {
      ShowIcons(true);
    }
  Client::Raise(mode);
}

void Desktop::GrabButtons(bool first) {
  if (first) {
    for (Client *c=firstchild; c; c=c->next)
      c->GrabButtons(true);
    return;
  }
  if (flags & CF_RAISEONCLICK)
    for (int i = 0; i!=3; ++i)
      if (LastRaised[i])
        LastRaised[i]->GrabButtons(first);
}


void Desktop::ShowIcons(bool raise) {
  if (raise && !(flags & DF_ICONSRAISED)) {
    for (Icon *i = firsticon;i; i = i->next) {
      XRaiseWindow(dpy,i->window);
      if (i->iconwin)
        XRaiseWindow(dpy,i->iconwin);
    }
    if (firsticon && (!UEH->CurrentIcon || UEH->CurrentIcon->parent != this)) {
      UEH->SetCurrent(0,firsticon);
    }
    flags |= DF_ICONSRAISED;
  } else
    if (flags & DF_ICONSRAISED) {
      for (Icon *i = firsticon;i; i = i->next) {
        XLowerWindow(dpy,i->window);
        if (i->iconwin)
          XLowerWindow(dpy,i->iconwin);
      }
      flags &= ~DF_ICONSRAISED;
    }
}


Icon *Desktop::AddIcon(Icon *i) {
  Icon *ii = firsticon;
  if (ii) {
    for (;ii->next;ii = ii->next);
    ii->next = i;
  } else {
    firsticon = i;
  }
  i->next = 0;
  return i;
}

void Desktop::RemoveIcon(Icon *i) {
  if (i==firsticon)
    firsticon = i->next; else
    for (Icon *ii=firsticon;ii->next;ii=ii->next)
      if (ii->next==i) {
        ii->next=i->next;
        break;
      }
  if (i->client)
    i->client->icon = 0;
  i->Remove();
  delete i;
}

void Desktop::NextIcon(Icon *i, int dir) {
  Icon *best = 0;
  switch (dir) {
    case NI_UP:
      for (Icon *ii = firsticon; ii; ii = ii->next) {
        if (ii == i)
          continue;
        if (ii->y >= i->y)
          continue;
        if (!best) {
          best = ii;
          continue;
        }
        if (ii->y > best->y)
          best = ii;
        if (ii->y == best->y && abs(ii->x-i->x) < abs(best->x-i->x))
          best = ii;
      }
      break;
    case NI_DOWN:
      for (Icon *ii = firsticon; ii; ii = ii->next) {
        if (ii == i)
          continue;
        if (ii->y <= i->y)
          continue;
        if (!best) {
          best = ii;
          continue;
        }
        if (ii->y < best->y)
          best = ii;
        if (ii->y == best->y && abs(ii->x-i->x) < abs(best->x-i->x))
          best = ii;
      }
      break;
    case NI_LEFT:
      for (Icon *ii = firsticon; ii; ii = ii->next) {
        if (ii == i)
          continue;
        if (ii->x >= i->x)
          continue;
        if (!best) {
          best = ii;
          continue;
        }
        if (ii->x > best->x)
          best = ii;
        if (ii->x == best->x && abs(ii->y-i->y) < abs(best->y-i->y))
          best = ii;
      }
      break;
    case NI_RIGHT:
      for (Icon *ii = firsticon; ii; ii = ii->next) {
        if (ii == i)
          continue;
        if (ii->x <= i->x)
          continue;
        if (!best) {
          best = ii;
          continue;
        }
        if (ii->x < best->x)
          best = ii;
        if (ii->x == best->x && abs(ii->y-i->y) < abs(best->y-i->y))
          best = ii;
      }
      break;


  };
  if (best)
    UEH->SetCurrent(0,best);

}


void Desktop::Take(Client *c, int x, int y) {
  c->parent=this;
  c->UpdateFlags(FLAG_UPDATE,0);
  if (firstchild) {
    Client *cc;
    for (cc=firstchild;cc->next;cc=cc->next);
    cc->next = c;
  } else {
    firstchild = c;
    focus = c; // give first window on this Desktop the focus
    focus->UpdateName(false); // we must be very careful with this call
  }
  c->next = NULL;
  if (c->MaxW || c->MaxH) {
    if (c->MaxW == c->width && c->MaxH == c->height) {
      c->MaxW = 0;
      c->MaxH = 0;
    } else c->Maximize(MAX_UNMAX);

  }
  c->x = x;
  c->y = y;
  Translate(&x,&y);
  XReparentWindow(dpy,c->frame,window,x,y - c->THeight);
  SendConfig();
  c->Raise(0);

  if (c->mapped && (c->flags & CF_HASTBENTRY))
    UpdateTB();
  if (ct->Focus == 0 || ct->Focus == this || ct->Focus == focus)
      man->WantFocus = c;
  Tile();
  AutoResize();

}


void Desktop::RequestDesktop(bool collect) {
  Desktop *d = new Desktop(this);
  AddDesktop(d,0);
  if (!d)
    return;
  d->MoveResize(0,true,0,0,true);
  d->MoveResize(S_BOTTOM | S_RIGHT,true,0,0);
  if (!collect)
    return;
  Client *last = 0, *i = firstchild;
  while (i) {
    if ((d != i) &&
        (i->x >= d->x) &&
        (i->y >= d->y) &&
        (d->x+d->width >= i->x) &&
        (d->y+d->height+i->THeight >= i->y)
       ) {
      GiveAway(i,false);
      d->Take(i,i->x - (d->x_root - x_root), i->y - (d->y_root - y_root));
      i = last ? last->next : firstchild;
    } else {
      last = i;
      i = i->next;
    }
  }
  UpdateTB();
  if (d->mapped && (d->flags & CF_HASTBENTRY))
    UpdateTB();
}

void Desktop::GetFocus() {
  ct->CurrentDesktop = this;
  if (parent) {
    if (parent->focus)
      parent->focus->Clear();;
    parent->focus=this;
    parent->Clear();
  }
  if (focus) {
//  The child will ask me to ReDraw() myself
    if (flags & DF_GRABKEYBOARD)
      XSetInputFocus(dpy,man->input,RevertToPointerRoot,CurrentTime); else
      focus->GetFocus();
  } else {
    Client::GetFocus();
    Clear();
  }
  UEH->SetCurrent(this,0);
}

Client *Desktop::Focus() {
  if (focus)
    return focus->Focus();
  return this;
}

/* returns true if the virtual position was changed */
bool Desktop::Goto(int x, int y, int mode) {
  if (ct->MaxWindow)
    return false;
  int oldPosX = PosX;
  int oldPosY = PosY;
  int diff;
// width - Sc->BW ???
// !!!
  width += Sc->BW;
  height += Sc->BW;
  // i hate this implemetation of the / - operator
  if (mode & GOTO_RELATIVE) {
    diff =  x;
  } else {
    if (x >= width - 2*Sc->BW)
      ++x;
    if (x < 0)
      --x;
    diff = ((x + width*VirtualX) / width - VirtualX) * width;
    // Actually, diff = (x / width) * width
  }
  if (diff) {
    PosX += diff;
    if ((mode & GOTO_SNAP) && (flags & CF_SNAP)) {
      diff = (PosX + snap) % width;
      if (diff <= 2*snap)
        PosX += snap - diff;
    }
    if (PosX < 0)
      PosX = 0;
    if (PosX > width*(VirtualX - 1))
      PosX = width*(VirtualX - 1);
  }
  if (mode & GOTO_RELATIVE) {
    diff = y;
  } else {
    if (y >= height - 2*Sc->BW)
      ++y;
    if (y < 0)
      --y;
    diff = ((y + height*VirtualY) / height - VirtualY) * height;
  }
  if (diff) {
    PosY += diff;
    if (mode & GOTO_SNAP) {
      diff = (PosY + snap) % height;
      if (diff <= 2*snap)
        PosY += snap - diff;
    }
    if (PosY < 0)
      PosY = 0;
    if (PosY > height*(VirtualY - 1))
      PosY = height*(VirtualY - 1);
  }

  width -= Sc->BW;
  height -= Sc->BW;
  if (PosX == oldPosX && PosY == oldPosY)
    return false;
  for (Client *i= firstchild;i;i=i->next)
    if (i->mapped && !(i->flags & CF_STICKY)) {
      i->x -= PosX - oldPosX;
      i->y -= PosY - oldPosY;
      i->ChangePos(true);
    }
  if (mode & (GOTO_MOVEFULL | GOTO_MOVEHALF)) {
    oldPosX -= PosX;
    oldPosY -= PosY;
    if (mode & GOTO_MOVEHALF) {
      oldPosX /= 2;
      oldPosY /= 2;
    }
    XWarpPointer(dpy,root,0,0,0,0,0,oldPosX,oldPosY);
    ct->CurrentDesktop = this;
  }
  return true;
}

bool Desktop::Leave(int x,int y,bool half) {
  return Goto(x - x_root,y - y_root,half ? GOTO_MOVEHALF : GOTO_MOVEFULL);
}


void Desktop::Tile() {
  if (!(flags & DF_TILE))
    return;
  Client *c;
  int n = 0;
  for (c = firstchild; c; c=c->next)
    if (c->mapped)
      ++n;
  if (!n)
    return;
  if (n == 1) {
    for (c = firstchild; c; c=c->next)
      if (c->mapped)
        break;
    c->x = - Sc->BW;
    c->y = THeight - Sc->BW;
    c->ChangePos(false);
  } else {
    RectRec rects[n];
    int idNo=0;
    for (c = firstchild; c; c=c->next)
      if (c->mapped) {
        rects[idNo].assign(c->width,c->height+c->THeight,idNo);
        ++idNo;
      }
    bool tilingOK;
    if (parent && (flags & DF_AUTORESIZE))
      tilingOK = tileArea(parent->width - (x % parent->width),
                          parent->height - (y % parent->height),idNo,rects); else
      tilingOK = tileArea(width,height,idNo,rects);
    if(tilingOK){
      /*this going be nasty because tileArea has sorted the rects list by
        area so we need to search for the correct entry*/
      idNo=0;
      for (c = firstchild; c; c=c->next)
        if (c->mapped) {
          int k=0;
          while(rects[k].idNo!=idNo){
            ++k;
          }
          c->x=rects[k].x - Sc->BW;
          c->y=rects[k].y+c->THeight - Sc->BW;
          c->ChangePos(false);
          ++idNo;
        }
    }
  }
  if (parent->flags & DF_AUTORESIZE)
    parent->AutoResize();
}


void Desktop::SetWMState(){
  for (Client *c=firstchild;c;c=c->next)
    if (c->mapped) {
      c->visible = visible;
      c->SetWMState();
    }

}


void Desktop::SendConfig() {
  Client::SendConfig();
  for (Client *c = firstchild; c; c=c->next) {
    c->SendConfig();
  }
}


Desktop::~Desktop() {
  Icon *savei;
  for (Icon *i = firsticon;i;i=savei) {
    savei = i->next;
    if (i->client)
      i->client->icon = 0;
    delete i;
  }
  for (Client *c=firstchild;c;firstchild = c) {
    c = firstchild->next;
    delete firstchild;
  }
}

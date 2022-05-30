/* treewm - an X11 window manager.
 * Copyright (c) 2001-2002 Thomas Jäger <TheHunter2000 at web dot de>
 * This code is released under the terms of the GNU GPL.
 * See the included file LICENSE for details.
 */

#include <X11/xpm.h>
#include "icon.h"
#include "desktop.h"
#include "sceme.h"
#include "action.h"
#include "clienttree.h"

Icon::Icon(Desktop *p,Client *c, char *aname) : client(c),parent(p) {
  flags = TF_ISICON;
  pixmap = 0;
  iconwin = 0;
  window = 0;
  name = 0;
  spm = 0;
  cmd = 0;
  Action *action = 0;
  if (aname && aname[0]=='@')
    action = (Action *)rman->GetInfo(SE_ACTION,aname+1);
  x = UNDEF;
  y = UNDEF;
  if (client && !action) {
    if (client->flags & CF_ICONPOSKNOWN) {
      x = client->IconX;
      y = client->IconY;
    }
  } else if (action) {
    if ((action->flags & AC_ICONX) && (action->flags & AC_ICONY)) {
      x = action->IconX;
      y = action->IconY;
    }
  }
  if (action)
    name = strdup(action->name);
  if (aname) {
    cmd = strdup(aname);
    if (!name)
      name = strdup(aname);
  }
  if (action)
    spm = action->iconpm; //dirty
}


Icon::Icon(Desktop *p,Icon *i, bool copy): parent(p) {
  flags = TF_ISICON;
  pixmap = 0;
  iconwin = 0;
  window = 0;
  name = 0;
  spm = 0;
  cmd = 0;
  x = UNDEF;
  y = UNDEF;
  client = i->client;
  if (i->cmd)
    cmd = strdup(i->cmd);
  if (i->name) {
    name = new char[strlen(i->name)+1];
    strcpy(name,i->name);
  }
  spm = i->spm;
  if (client && !cmd && copy) {
    cmd = strdup("g");
  }

}

void Icon::Init() {
  if (x==UNDEF || y==UNDEF)
    GetPosition();
  ChangeName(name);
  XSetWindowAttributes attr;
  attr.override_redirect = true;
  attr.background_pixel = parent->Sc->colors[C_IBG].pixel;
  attr.border_pixel = parent->Sc->colors[C_IBD].pixel ;
  attr.event_mask = (ButtonMask | ExposureMask);
  window = XCreateWindow(dpy, parent->window, x - w/2, y, w, h,1,
                    CopyFromParent,
                    CopyFromParent,CopyFromParent,CWBorderPixel | CWEventMask | CWBackPixel | CWOverrideRedirect,&attr);
  attr.background_pixel = parent->Sc->colors[C_IBG].pixel;
  if (spm)
    GetXPMFile(spm,&attr); // dirty
  if (!iconwin && client && client->iconpm)
    GetXPMFile(client->iconpm,&attr);
  if (!iconwin && client) {
    Client *focus = client->Focus();
    if (focus) {
      XWMHints *hints = XGetWMHints(dpy, focus->window);
      if (hints) {
        if(hints->flags & IconWindowHint) {
          GetIconWindow(hints);
        } else
          if ((hints->flags & IconPixmapHint)) {
            GetIconBitmap(hints,&attr);
          }
        XFree(hints);
      }
    }
  }
  if (!iconwin && client && client->Sc->iconpm)
    GetXPMFile(client->Sc->iconpm,&attr);

  if (parent->flags & DF_ICONSRAISED) {
    XRaiseWindow(dpy,window);
    if (iconwin)
      XRaiseWindow(dpy,iconwin);
  } else {
    XLowerWindow(dpy,window);
    if (iconwin)
      XLowerWindow(dpy,iconwin);
  }
  XMapWindow(dpy,window);
  if (iconwin)
    XMapWindow(dpy,iconwin);
  if (window)
    ct->windows.insert(WmapPair(window,this));
  if (iconwin)
    ct->windows.insert(WmapPair(iconwin,this));



}


inline int Min(int x,int y) {
  return (x < y) ? x : y;
}

// we are lazy
#define MINIX        MinIX
#define MINIY        MinIY
#define GRIDX        parent->Sc->GridX
#define GRIDY        parent->Sc->GridY
#define ISPACELEFT   parent->Sc->ISLeft
#define ISPACERIGHT  parent->Sc->ISRight
#define ISPACETOP    parent->Sc->ISTop
#define ISPACEBOTTOM parent->Sc->ISBottom

void Icon::GetPosition() {
  MinIX = parent->Sc->MinIX;
  MinIY = parent->Sc->MinIY;
  parent->Translate(&MinIX,&MinIY);
  int nx = (parent->width - MINIX) / GRIDX;
  int ny = (parent->height - MINIY) / GRIDY;
  x=0; y=0;
// avoid almost-endless loops
  if (nx > 200)
    nx = 200;
  if (ny > 200)
    ny = 200;
  if (nx < 0)
    nx = 1;
  if (ny < 0)
    ny = 1;

  bool Grid[nx][ny];
  for (int i = 0;i!=nx;++i)
    for (int j = 0;j!=ny;++j)
      Grid[i][j] = true;

  for (Icon *ii=parent->firsticon;ii;ii=ii->next) {
    if (ii == this)
      continue;
    int x1 = (ii->x - Min(ii->w/2,ISPACELEFT)
              - MINIX - ISPACERIGHT  + GRIDX-1) / GRIDX; // the / - operator sucks with negative numbers
    int y1 = (ii->y - Min((ii->iconwin ? ii->iconh : 0),ISPACETOP)
              - MINIY - ISPACEBOTTOM + GRIDX-1) / GRIDY;
    int x2 = (ii->x + Min(ii->w/2,ISPACERIGHT)
              - MINIX + ISPACELEFT) / GRIDX;
    int y2 = (ii->y + Min(ii->h,ISPACEBOTTOM)
              - MINIY + ISPACETOP) / GRIDY;
    if (x1 < 0)
      x1 = 0;
    if (y1 < 0)
      y1 = 0;
    ++x2;++y2;
    if (x2 > nx)
      x2 = nx;
    if (y2 > ny)
      y2 = ny;
    for (int i = x1;i<x2;++i)
      for (int j = y1;j<y2;++j)
        Grid[i][j] = false;
  }

  for (int i = 0;i<nx+ny-1;++i)
    for (int j = 0;j!=i+1;++j)
      if (j<nx && i-j<ny && Grid[j][i-j]) {
        x = j*GRIDX + MINIX;
        y = (i-j)*GRIDY + MINIY;
        goto next; // ugly
      }
  next:
  if (!x)
    x = MINIX;
  if (!y)
    y = MINIY;

  for (Client *cc=parent->firstchild;cc;cc=cc->next)
    if (cc->mapped) {
      int x1 = (cc->x -               cc->Sc->BW - MINIX - ISPACERIGHT + GRIDX - 1) / GRIDX;
      int y1 = (cc->y - cc->THeight - cc->Sc->BW - MINIY - ISPACEBOTTOM + GRIDY - 1) / GRIDY;
      int x2 = (cc->x + cc->width   + cc->Sc->BW - MINIX + ISPACELEFT) / GRIDX;
      int y2 = (cc->y + cc->height  + cc->Sc->BW - MINIY + ISPACETOP) / GRIDY;
      if (x1 < 0)
        x1 = 0;
      if (y1 < 0)
        y1 = 0;
      ++x2;++y2;
      if (x2 > nx)
        x2 = nx;
      if (y2 > ny)
        y2 = ny;
      for (int i = x1;i<x2;++i)
        for (int j = y1;j<y2;++j) {
          Grid[i][j] = false;
        }
    }

  for (int i = 0;i<nx+ny-1;++i)
    for (int j = 0;j!=i+1;++j)
      if (j<nx && i-j<ny && Grid[j][i-j]) {
        x = j*GRIDX + MINIX;
        y = (i-j)*GRIDY + MINIY;
        return;
      }
}

void Icon::Move() {
  int dx = (x + snap - MINIX) % GRIDX - snap;
  int dy = (y + snap - MINIY) % GRIDY - snap;
  if (abs(dx)<=snap && abs(dy)<=snap) {
    x-=dx;
    y-=dy;
  }
  XMoveWindow(dpy, window,x - w/2,y);
  if (iconwin)
    XMoveWindow(dpy, iconwin, x-iconw/2, y-iconh-2);
}


void Icon::ChangeName(char *newname) {
  char *str = 0;
  char *iconname = 0;
  if (!name || newname != name) {
    if (!newname && client && !cmd) {
      str = client->name ? client->name : client->wmname;
      XGetIconName(dpy,client->window,&iconname);
      if (!iconname || !iconname[0])
        newname = 0; else
        newname = iconname;
    }
    if (!newname)
      newname = "Icon";
    char save = '\0';
    if (strlen(newname) > MAXICON)
      save = newname[MAXICON];
    name = strdup(newname);
    if (save)
      newname[MAXICON] = save;
    if (name && str && !strcmp(name, str)) {
      free(name); // Save memory
      name = 0;
    }
    if (iconname)
      XFree(iconname);
  }
  str = name ? name : str;
  XFontSetExtents *e;
  e = XExtentsOfFontSet(parent->Sc->fonts[FO_ICON]);
  w = XmbTextEscapement(parent->Sc->fonts[FO_ICON], str, strlen(str)) + 2*SPACE;
  h = e->max_logical_extent.height*4/5 + e->max_logical_extent.height/5 + 2*SPACE;
  if (window)
    XMoveResizeWindow(dpy, window, x - w/2, y, w, h);

}


void Icon::GetIconBitmap(XWMHints *hints,XSetWindowAttributes *attr) {
  int dummyx,dummyy;
  unsigned int dummybw;
  Window dummyroot;
  XGetGeometry(dpy, hints->icon_pixmap, &dummyroot, &dummyx, &dummyy,
	       (unsigned int *)&iconw,
	       (unsigned int *)&iconh, &dummybw, &depth);
  pixmap = hints->icon_pixmap;
#ifdef SHAPE
/*  if (hints->flags & IconMaskHint)
    {
      tmp_win->icon_maskPixmap = hints->icon_mask;
    }*/
#endif
  iconwin =	XCreateWindow(dpy, parent->window, x-iconw/2, y-iconh-2, iconw,
  		      iconh, 0, CopyFromParent,
  		      CopyFromParent,CopyFromParent,CWBorderPixel | CWEventMask | CWBackPixel | CWOverrideRedirect,attr);
  		
}

void Icon::GetIconWindow(XWMHints *hints) {
  int dummyx,dummyy;
  unsigned int dummybw;
  Window dummyroot;
  iconwin = hints->icon_window;
  XGetGeometry(dpy, iconwin, &dummyroot, &dummyx, &dummyy,
	       (unsigned int *)&iconw,
	       (unsigned int *)&iconh, &dummybw, &depth);
	iconh +=4; // windows seem to supply their own border
  pixmap = 0;// hints->icon_pixmap;
  flags |= IF_APPICON;
  XReparentWindow(dpy,iconwin,parent->window,x-iconw/2,y-iconh-2);
  XAddToSaveSet(dpy,iconwin);
  XMapWindow(dpy,iconwin);
  XSelectInput(dpy,iconwin,(ButtonPressMask | ButtonReleaseMask | Button2MotionMask));
}


bool Icon::GetXPMFile(RPixmap *pm,XSetWindowAttributes *attr) {
  pixmap = pm->GetPixmap();
  if (!pixmap)
    return false;
  spm = pm;
  iconw = pm->w;
  iconh = pm->h;
  depth = DefaultDepth(dpy,screen);
#ifdef SHAPE
/*      if (tmp_win->icon_maskPixmap)
	tmp_win->flags |= SHAPED_ICON;
*/	
#endif

  iconwin =	XCreateWindow(dpy, parent->window, x-iconw/2, y-iconh-2, iconw,
  		      iconh, 0, CopyFromParent,
  		      CopyFromParent,CopyFromParent,CWBorderPixel | CWEventMask | CWBackPixel | CWOverrideRedirect,attr);
	return true;
}


void Icon::ReDraw() {
  if (!window || !parent->visible)
    return;
  char *str;

  if (client) {
     str = name ? name : (client->name ? client->name : client->wmname);
  } else
    str = name;
  XFontSetExtents *e;
  e = XExtentsOfFontSet(parent->Sc->fonts[FO_ICON]);
  XmbDrawString(dpy, window, parent->Sc->fonts[FO_ICON], parent->Sc->icon_gc,
    SPACE, SPACE + e->max_logical_extent.height*4/5,
    str, strlen(str));

  if (pixmap != None) {
    if (depth == (unsigned int)DefaultDepth(dpy,screen)) {
      XCopyArea(dpy,pixmap,iconwin,parent->Sc->icon_gc,0,0,iconw-4,iconh-4,2,2);
    } else {
      XCopyPlane(dpy,pixmap,iconwin,parent->Sc->icon_gc,0,0,iconw-4,iconh-4,2,2,1);
    }
  }


}

void Icon::Execute() {
  parent->Client::GetFocus(); //!!!!!
  if (cmd)
    UEH->ExecCommand(client,0,cmd,true);
  if (client && !cmd)
    client->Map();
}


bool Icon::IsIn(Window win, int xx, int yy) {
  if (win == iconwin) {
    return (0<=xx && xx<=iconw && 0<=yy && yy<=iconh);
  }
  if (win == window) {
    return (0<=xx && xx<=w && 0<=yy && yy<=h);
  }
  return false;
}

void Icon::Remove() {
  if (window) {
    ct->windows.erase(window);
    XUnmapWindow(dpy,window);
    XDestroyWindow(dpy, window);
    window = 0;
  }
  if (iconwin) {
    ct->windows.erase(iconwin);
    XUnmapWindow(dpy,iconwin);
    if (flags & IF_APPICON) {
      XReparentWindow(dpy,iconwin,root,0,0);
      XUnmapWindow(dpy,iconwin);
    } else {
      XDestroyWindow(dpy, iconwin);
    }
    iconwin = 0;
  }
  if (spm)
    spm->FreePixmap();
  if (UEH->CurrentIcon == this) {
    UEH->CurrentIcon = 0;
    UEH->Current = ct->RootDesktop;
  }
}

Icon::~Icon() {
  if (client && !cmd) {
//  x = i*GRIDX + MINIX;
//  y = i*GRIDY + MINIY;
    if (((x-MINIX) % GRIDX) || ((y-MINIY) % GRIDY)) {
      client->flags |= CF_ICONPOSKNOWN;
      client->IconX = x;
      client->IconY = y;
    } else {
      client->flags &= ~CF_ICONPOSKNOWN;
    }
  }
  if (name)
    free(name);
  if (cmd)
    free(cmd);
}

/* treewm - an X11 window manager.
 * Copyright (c) 2001-2002 Thomas Jäger <TheHunter2000 at web dot de>
 * This code is released under the terms of the GNU GPL.
 * See the included file LICENSE for details.
 */

#include "clienttree.h"
#include "sceme.h"
#include "clientinfo.h"
#include "manager.h"
#include "uehandler.h"
#include "icon.h"
#include "action.h"
#include <stdlib.h>
#include <X11/Xatom.h>


ClientTree *ct;

ClientTree::ClientTree() : windows() {

  XSetWindowAttributes pattr;

  notitle = new char[9];
  strcpy(notitle,"No Title");

  MaxWindow = 0;
  Focus = 0;
  numcmd = 0;
  numregex = 0;
  numwmclass = 0;

  ct = this;
  watch = 0;
  int w = DisplayWidth(dpy,screen);
  int h = DisplayHeight(dpy, screen);
  RootDesktop = new Desktop(0);
  ClientInfo *ci = (ClientInfo *)rman->GetInfo(SE_CLIENTINFO, "root");
  if (ci)
    RootDesktop->SetClientInfo(ci);
  RootDesktop->next = 0; // Very important
  UEH->SetCurrent(RootDesktop,0);
  CurrentDesktop = RootDesktop;
  for (int i=0;i!=4;++i)
    RootBorder[i] = 0;

  RootDesktop->Init();
  ct->windows.insert(WmapPair(RootDesktop->window,RootDesktop));
  Sceme *Sc = RootDesktop->Sc;
  XDefineCursor(dpy, root,Sc->cursors[CU_STD]);
  if (Sc->BW <= 0) {
    for (int i=0;i!=4;++i)
      RootBorder[i] = 0;
    return;
  }
  pattr.override_redirect = true;
  pattr.background_pixel = Sc->colors[C_BD].pixel;
  pattr.event_mask = EnterWindowMask;
  RootBorder[0] = XCreateWindow(dpy, root,
      0,0,w,Sc->BW, 0,
      DefaultDepth(dpy, screen), CopyFromParent, DefaultVisual(dpy, screen),
      CWOverrideRedirect|CWEventMask|CWBackPixel, &pattr);
  RootBorder[1] = XCreateWindow(dpy, root,
      0,0,Sc->BW,h, 0,
      DefaultDepth(dpy, screen), CopyFromParent, DefaultVisual(dpy, screen),
      CWOverrideRedirect|CWEventMask|CWBackPixel, &pattr);
  RootBorder[2] = XCreateWindow(dpy, root,
      0,h-Sc->BW,w,Sc->BW, 0,
      DefaultDepth(dpy, screen), CopyFromParent, DefaultVisual(dpy, screen),
      CWOverrideRedirect|CWEventMask|CWBackPixel, &pattr);
  RootBorder[3] = XCreateWindow(dpy, root,
      w-Sc->BW,0,Sc->BW,h, 0,
      DefaultDepth(dpy, screen), CopyFromParent, DefaultVisual(dpy, screen),
      CWOverrideRedirect|CWEventMask|CWBackPixel, &pattr);
  for (int i=0;i!=4;++i)
    XMapWindow(dpy,RootBorder[i]);
}

ClientTree::~ClientTree() {
  RootDesktop->Remove(R_REMAP);
  while (watch) {
    WatchList *nw = watch->next;
    if (watch->preg) {
      regfree(watch->preg);
      delete watch->preg;
    }
    delete watch;
    watch = nw;
  }

  delete RootDesktop;
  delete [] notitle;
  for (int i=0;i!=4;++i)
    if (RootBorder[i])
      XDestroyWindow(dpy,RootBorder[i]);
  
}

Client *ClientTree::FindClient(Window win, Icon **icon) {
  WmapIter i = windows.find(win);
  if (i == windows.end())
    return 0;
  if (i->second->flags & TF_ISICON) {
    if (icon)
      (*icon) = (Icon *)i->second;
    return 0;
  } else {
    return (Client *) i->second;
  }
}

void ClientTree::RemoveClientReferences(Client *c) {
  if (CurrentDesktop == c)
    CurrentDesktop = 0;
  if (Focus == c)
    Focus = 0;
  if (man->WantFocus == c)
    man->WantFocus = 0;
  if (man->ToBeRaised == c)
    man->ToBeRaised = 0;
  for (WatchList *i = watch;i;i=i->next)
    if (i->parent == c)
      i->parent = 0;
  for (ClientList *cl=man->FirstReDraw;cl;cl=cl->next)
    if (cl->client == c)
      cl->client = 0;
  for (ClientList *cl=man->MustBeDeleted;cl;cl=cl->next)
    if (cl->client == c)
      cl->client = 0;



  UEH->RemoveClientReferences(c);
  RootDesktop->RemoveClientReferences(c);
}


char *ClientTree::GetWindowCommand(Window win) {
  Atom real_type;
  int real_format;
  unsigned long items_read, items_left;

  Window *w;
  if (XGetWindowProperty(dpy, win, wm_client_leader, 0L, 1L, false,
          AnyPropertyType, &real_type, &real_format, &items_read, &items_left,
          (unsigned char **) &w) == Success && items_read) {
    win = *w;
    XFree(w);
  }
  char **argv;
  int argc;
  if (XGetCommand(dpy,win,&argv,&argc)) {
    char *ret = new char[strlen(argv[0])+1];
    strcpy(ret,argv[0]);
    XFreeStringList(argv);
    return ret;
  }
  return 0;

}

bool CompareClass(char *str,XClassHint hint) {
  int i=1;
  if (str[0] != '*') {
    if (!hint.res_name)
      return false;
    i = strlen(hint.res_name);
    if (strncasecmp(str,hint.res_name,i))
      return false;
  }
  if (str[i] != '.')
    return false;
  if (str[++i] != '*') {
    if (!hint.res_class)
      return false;
    if (strcasecmp(str+i,hint.res_class))
      return false;
  }
  return true;
}

WatchList *ClientTree::GetWatches(char *cmd, char *wname, XClassHint wmclass, Desktop *&d) {
  WatchList *prev = 0;
  WatchList *ret = 0;
  int dummy = 0;
  if (!watch)
    return 0;
  for (WatchList *i = watch;i;) {
    if ( (i->cmd ?  (cmd   && !strcmp(cmd,i->cmd)) : true) && // cmd is ok
         (i->preg ? (wname && !regexec(i->preg,wname,0,0,dummy)) : true) && // preg is ok
         (i->wmclass ? CompareClass(i->wmclass,wmclass) : true) // wmclass is ok
       ) {
      if (i->flags & AC_KEEP) {
        WatchList *oret = ret;
        ret = new WatchList;
        ret->next = oret;
        ret->ci = i->ci;
        ret->cmd = i->cmd;
        ret->flags = i->flags;
        ret->parent = i->parent;
        ret->preg = i->preg;
        ret->wmclass = i->wmclass;
        prev = i;
        i = i->next;
      } else {
        WatchList *oret = ret;
        ret = i;
        if (prev)
          prev->next = i->next; else
          watch = i->next;
        ret->next = oret;
        if (i->cmd)
          --numcmd;
        if (i->wmclass)
          --numwmclass;
        if (i->preg) {
          regfree(i->preg);
          --numregex;
          delete i->preg;
        }
        if (prev)
          i = prev->next; else
          i = watch;
      }
    } else { // doesn't match
      prev = i;
      i = i->next;
    }
  }

  return ret;
}


Client* ClientTree::AddWindow(Window win) {
  bool desktop = false;
  Desktop *d = 0;


  char *cmd = 0;
  char *wname = 0;
  XClassHint wmclass;
  if (numcmd)
    cmd = GetWindowCommand(win);
  if (numregex)
    FetchName(win,&wname);
  if (numwmclass ? !XGetClassHint(dpy,win,&wmclass) : true) {
    wmclass.res_name = 0;
    wmclass.res_class = 0;
  }

  WatchList *wl = GetWatches(cmd,wname,wmclass,d);
  if (wname)
    XFree(wname);
  if (wmclass.res_name)
    XFree(wmclass.res_name);
  if (wmclass.res_class)
    XFree(wmclass.res_class);

  if (!d) {
    d = CurrentDesktop;
    if (!d)
      d = RootDesktop;
  }
  Window tfor;
  Client *transfor;
  // Sorry that we have to call this function twice
  // since Xlib functions that return a value are extremely slow
  XGetTransientForHint(dpy, win, &tfor);
  // but we'll have to put the window onto the same desktop as transfor

  if (tfor) { // don't check watches at all when window has a transient???
    transfor=ct->FindClient(tfor,0);
    if (transfor) {
      for (;transfor->trans;transfor=transfor->trans);
      // they are really applications that  make themselves
      // transient for the root window - no comment
      if (transfor->parent)
        d = transfor->parent;
    }
  } else
    transfor = NULL;

  Client *c = new Client(d,win);
  while (wl) {
    c->SetClientInfo(wl->ci);
    if (wl->flags & AC_CLIENTDESKTOP)
      desktop = true;
    WatchList *nextwl = wl->next;
    delete wl;
    wl = nextwl;
  }
  d->AddChild(c);
  if (desktop && !ISDESKTOP(c))
    c->RequestDesktop(false);
  if (MaxWindow)
    c->Maximize(MAX_FULLSCREEN);
  return c;
}


void ClientTree::AddWatch(Desktop *p,ClientInfo *ci,char *cmd,char *regex,
                          char *wmclass, long flags) {
  WatchList *wl = watch;
  watch = new WatchList;
  watch->next = wl;
  watch->ci = ci;
  watch->parent = p;
  watch->cmd = cmd;
  watch->wmclass = wmclass;
  watch->flags = flags;
  if (cmd)
    ++numcmd;
  if (wmclass)
    ++numwmclass;
  if (regex) {
    watch->preg = new regex_t;
    regcomp(watch->preg,regex,REG_NOSUB|REG_ICASE|REG_EXTENDED);
    ++numregex;
  } else
    watch->preg = 0;

}

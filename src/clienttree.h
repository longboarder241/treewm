/* treewm - an X11 window manager.
 * Copyright (c) 2001-2002 Thomas Jäger <TheHunter2000 at web dot de>
 * This code is released under the terms of the GNU GPL.
 * See the included file LICENSE for details.
 * 
 * changed<05.02.2003> by Rudolf Polzer: replaced hash_map by map (for gcc 3.2)
 */

#ifndef CLIENTTREE_H
#define CLIENTTREE_H

#include "global.h"
#include "desktop.h" // because of inline functions
#include "manager.h"
#include "uehandler.h"
#include <map>
#include <regex.h>

struct WatchList {
  ClientInfo *ci;
  char *cmd;
  Desktop *parent;
  regex_t *preg;
  char *wmclass;
  long flags;
  WatchList *next;
};

typedef map<Window,TWindow *> Wmap;
typedef Wmap::value_type WmapPair;
typedef Wmap::iterator WmapIter;


class ClientTree {
  public:
    ClientTree();
    ~ClientTree();
    Client *AddWindow(Window);
    char *GetWindowCommand(Window);
    WatchList *GetWatches(char *, char *,XClassHint, Desktop *&);
    void AddWatch(Desktop *,ClientInfo *,char *,char *,char *,long);
    Client *FindClient(Window, Icon **);
    void RemoveClientReferences(Client *);
    inline Client *FindPointerClient();
    Wmap windows;
    Desktop *RootDesktop,*CurrentDesktop;
    Client *Focus;
    Client *MaxWindow;
    char *notitle;
    char *rootname;
    int numregex,numcmd,numwmclass;
    WatchList *watch;
    Window RootBorder[4];
#ifdef VIDMODE
    int ctVX,ctVY,ctVHeight,ctVWidth;
#endif
};

inline Client *ClientTree::FindPointerClient() {
  return RootDesktop->FindPointerClient();
};


extern ClientTree *ct;

#endif

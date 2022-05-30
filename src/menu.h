/* treewm - an X11 window manager.
 * Copyright (c) 2001-2003 Thomas Jäger <TheHunter2000 at web dot de>
 * This code is released under the terms of the GNU GPL.
 * See the included file LICENSE for details.
 */

#ifndef MENU_H
#define MENU_H

#include "global.h"

#define MENUSPACE 2
#define MENUINDENT (EHeight/2)

#define MF_INDENT 16

struct MenuItem;

struct SubMenuInfo {
  MenuItem *menu;
  int num;
};

struct MenuItem {
  char *text;
  char *key;
  unsigned int flags;
  Client *client;
  SubMenuInfo *submenu;
};

class Menu : public TWindow  {
  public:
    Menu(Menu *, const MenuItem*, int, Client *, char *, Sceme *, int, int, bool);
    virtual ~Menu();
    virtual void Init();
    void Mouse(int,int);
    bool Key(XKeyEvent &);
    void DrawItem(int);
    void ReDraw();
    bool Remove(int *);
    char *GetCmd();
    Menu *parent;
    Sceme *Sc;
    Window window;
    Client *client;
    char *cmd;
    char *buf;
    int selsubmenu;
    Menu *submenu;
    int x,y,width,height;
    int ignore;
    int n,sel;
    int EHeight;
    const MenuItem *items;
};

#endif

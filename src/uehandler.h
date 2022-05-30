/* treewm - an X11 window manager.
 * Copyright (c) 2001-2002 Thomas Jäger <TheHunter2000 at web dot de>
 * This code is released under the terms of the GNU GPL.
 * See the included file LICENSE for details.
 * 
 * changed<05.02.2003> by Rudolf Polzer: replaced hash_map by map (C++)
 */

#ifndef UEHANDLER_H
#define UEHANDLER_H

#include "global.h"
#include "client.h"
#include <regex.h>
#include <map>

#define RESIZERATIO 5
#define RESIZEMIN 40
#define RESIZEMAX 20

struct ModList {
  unsigned int mod;
  char *cmd;
  ModList *next;
};

typedef map<KeyCode,ModList *> KeyMap;
typedef KeyMap::iterator KeyMapIter;

class UEHandler {
  public:
	  UEHandler();
    ~UEHandler();
    void AddKey(KeySym,unsigned int,char *);
    void Press(XButtonEvent &);
    void Motion(XMotionEvent &);
    void Release(XButtonEvent &);
    void Key(XKeyEvent &);
    void RemoveClientReferences(Client *);
    void SetCurrent(Client *,Icon *);
    bool ExecCommand(Client *, Icon *,char *,bool,bool = false);
    void ExecCommands(char *);
    void ExecTextCommand(Client *,char *,bool);
    Desktop *SelDesktop;
    Client *SelEntry;
    Client *SelClient;
    Client *RefClient;
    Menu *menu;
    MenuItem *menuitems;
    unsigned long Seltype;
    TextDialog *dialog;
    Client *Current;
    Icon *CurrentIcon;
  protected:
    KeyMap keys;
    KeyCode AltSpace;
    Client *reg['z'-'a'+1];
    Client *dreg;
    Window MoveWin;
    int xdown,ydown;
    Client *NewTarget;
    Icon *SelIcon;
    Time downtime;
    bool motion;
    regex_t *preg;
    	
};

#endif

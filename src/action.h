/* treewm - an X11 window manager.
 * Copyright (c) 2001-2002 Thomas Jäger <TheHunter2000 at web dot de>
 * This code is released under the terms of the GNU GPL.
 * See the included file LICENSE for details.
 */

#ifndef ACTION_H
#define ACTION_H

#include "global.h"
#include "resmanager.h"

#define AC_ICONX           1
#define AC_ICONY           2
#define AC_AUTOSTART       4
#define AC_DESKTOP         8
#define AC_HAS_WM_COMMAND 16
#define AC_KEEP           32
#define AC_CLIENTDESKTOP  64

void Fork(char *);

class Action : public Info {
  public:
    Action(Section *);
    virtual ~Action();
    virtual void Init();
    void Execute();
    RPixmap *iconpm;
    int IconX,IconY;
    char *cmd,*name,*regex,*wmclass;
    long flags;
    ClientInfo *clientinfo;

    Section *s;
};

#endif

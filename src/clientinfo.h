/* treewm - an X11 window manager.
 * Copyright (c) 2001-2002 Thomas Jäger <TheHunter2000 at web dot de>
 * This code is released under the terms of the GNU GPL.
 * See the included file LICENSE for details.
 */

#ifndef CLIENTINFO_H
#define CLIENTINFO_H

#include "global.h"
#include "resmanager.h"


// defines for flags
#define CI_X                       (1L<< 0)
#define CI_Y                       (1L<< 1)
#define CI_WIDTH                   (1L<< 2)
#define CI_HEIGHT                  (1L<< 3)
#define CI_ICONX                   (1L<< 5)
#define CI_ICONY                   (1L<< 6)
#define CI_VIRTUALX                (1L<< 7)
#define CI_VIRTUALY                (1L<< 8)
#define CI_NOBACKGROUND            (1L<< 9)

struct ActionList {
  char *a;
  ActionList *next;
};

class ClientInfo : public Info {
  public:
    ClientInfo(Section *);
    virtual ~ClientInfo();
    virtual void Init();
    char	*name;
    int	x, y, width, height;
    int IconX,IconY;
    int VirtualX, VirtualY;
    RPixmap *iconpm, *bgpm;
    unsigned long flags;
    unsigned long clientflags;
    unsigned long clientmask;
    Sceme *Sc;
    ActionList *actions;

    Section *s;

};

#endif

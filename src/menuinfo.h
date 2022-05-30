/* treewm - an X11 window manager.
 * Copyright (c) 2001 Thomas Jäger <TheHunter2000 at web dot de>
 * This code is released under the terms of the GNU GPL.
 * See the included file LICENSE for details.
 */

#ifndef MENUINFO_H
#define MENUINFO_H

#include "resmanager.h"
#include "menu.h"

class MenuInfo : public Info  {
  public:
    MenuInfo(Section *);
    virtual void Init();
    ~MenuInfo();
    MenuItem *menu;
    int n;
  protected:
    Section *s;
};

#endif

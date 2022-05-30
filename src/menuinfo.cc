/* treewm - an X11 window manager.
 * Copyright (c) 2001-2002 Thomas Jäger <TheHunter2000 at web dot de>
 * This code is released under the terms of the GNU GPL.
 * See the included file LICENSE for details.
 */

#include "menuinfo.h"
#include "action.h"

struct MenuItemList {
  MenuItem mi;
  MenuItemList *next;
};

MenuInfo::MenuInfo(Section *section) {
  s = section;
}

void MenuInfo::Init() {
  n = 0;
  MenuItemList *actions=0,*last=0;
  for (Entry *e=s->fe;e;e=e->next)
    if (e->tag) {
      lower(e->value);
      if (last) {
        last->next = new MenuItemList;
        last = last->next;
      } else {
        actions = new MenuItemList;
        last = actions;
      }
      last->mi.text = strdup(e->tag);
      last->mi.key = strdup(e->value);
      last->mi.client = 0;
      last->mi.flags = 0;
      last->mi.submenu = 0;
      last->next = 0;
      ++n;
    }
  if (!n) {
    menu = 0;
    return;
  }
  menu = new MenuItem[n];
  int k=0;
  while (actions) {
    menu[k] = actions->mi;
    last = actions;
    actions = actions->next;
    delete last;
    ++k;
  }
}

MenuInfo::~MenuInfo() {
  for (int i=0; i!=n; ++i) {
    free(menu[i].text);
    free(menu[i].key);
  }
  if (menu)
    delete [] menu;
}

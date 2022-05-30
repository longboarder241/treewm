/* treewm - an X11 window manager.
 * Copyright (c) 2001-2002 Thomas Jäger <TheHunter2000 at web dot de>
 * This code is released under the terms of the GNU GPL.
 * See the included file LICENSE for details.
 */

#include "clientinfo.h"
#include <string.h>

ClientInfo::ClientInfo(Section *section) {
  s = section;
}

void ClientInfo::Init() {
  char *c;
  int v;
  flags = 0;
  clientflags = 0;
  clientmask = 0;
  c = s ? s->GetEntry("Name") : 0;
  if (c) {
    name = new char[strlen(c)+1];
    strcpy(name,c);
  } else name = 0;
  if (s && s->GetIntEntry("X",v)) {
    flags |= CI_X;
    x = v;
  }
  if (s && s->GetIntEntry("Y",v)) {
    flags |= CI_Y;
    y = v;
  }
  if (s && s->GetIntEntry("Width",v)) {
    flags |= CI_WIDTH;
    width = v;
  }
  if (s && s->GetIntEntry("Height",v)) {
    flags |= CI_HEIGHT;
    height = v;
  }
  if (s && s->GetIntEntry("XIcon",v)) {
    flags |= CI_ICONX;
    IconX = v;
  }
  if (s && s->GetIntEntry("XIcon",v)) {
    flags |= CI_ICONY;
    IconY = v;
  }
  if (s && s->GetIntEntry("VirtualX",v) && v>=1) {
    flags |= CI_VIRTUALX;
    VirtualX = v;
  }
  if (s && s->GetIntEntry("VirtualY",v) && v>=1) {
    flags |= CI_VIRTUALY;
    VirtualY = v;
  }
  c = s ? s->GetEntry("IconPath") : 0;
  if (c) {
    iconpm = rman->GetPixmap(c);
  } else iconpm = 0;

  c = s ? s->GetEntry("BGPixmap") : 0;
  if (c) {
    bgpm = rman->GetPixmap(c);
  } else bgpm = 0;

  c = s ? s->GetEntry("Sceme") : 0;
  if (c) {
    lower(c);
    Sc = (Sceme *)rman->GetInfo(SE_SCEME,c);
  } else
    Sc = 0;

  if (s && s->GetIntEntry("NoBackGround",v) && v)
    flags |= CI_NOBACKGROUND;

  if (s) {
    for (int i=0;i!=CLIENTFLAGNUM;++i)
      if (s->GetIntEntry(ClientFlagArray[i].str,v)) {
        clientmask |= ClientFlagArray[i].flag;
        if (v)
          clientflags |= ClientFlagArray[i].flag;
      }

  }

  actions = 0;
  ActionList *last=0;
  for (Entry *e=s->fe;e;e=e->next)
    if (!strncasecmp("Action",e->tag,6)) {
//      lower(e->value);
//      if (!rman->GetInfo(SE_ACTION,e->value))
//        continue;
      // Add action
      if (last) {
        last->next = new ActionList;;
        last = last->next;
      } else {
        actions = new ActionList;
        last = actions;
      }
      last->a = strdup(e->value);
      last->next = 0;
    }

}

ClientInfo::~ClientInfo() {
  ActionList *as;
  for (;actions;actions=as) {
    if (actions->a)
      free(actions->a);
    as = actions->next;
    delete actions;
  }
  delete [] name;
}

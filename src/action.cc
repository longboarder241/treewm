/* treewm - an X11 window manager.
 * Copyright (c) 2001-2002 Thomas Jäger <TheHunter2000 at web dot de>
 * This code is released under the terms of the GNU GPL.
 * See the included file LICENSE for details.
 */

#include "action.h"
#include "resmanager.h"
#include "clienttree.h"
#include "clientinfo.h"
#include <unistd.h>
#include <string.h>

Action::Action(Section *section) {
  s = section;
}

void Action::Init() {
  int v;
  char *c;
  flags = 0;
  iconpm = 0;
  cmd = 0;

  if (s && s->GetIntEntry("XIcon",v)) {
    flags |= AC_ICONX;
    IconX = v;
  }
  if (s && s->GetIntEntry("YIcon",v)) {
    flags |= AC_ICONY;
    IconY = v;
  }

  c = s ? s->GetEntry("IconPath") : 0;
  if (c) {
    iconpm = rman->GetPixmap(c);
  }

  c = s ? s->GetEntry("Name") : 0;
  c = c ? c : (char *)"Icon";
  if (c) {
    name = new char[strlen(c)+1];
    strcpy(name,c);
  }


  if (s && s->GetIntEntry("AutoStart",v))
    if (v)
      flags |= AC_AUTOSTART;

  if (s && s->GetIntEntry("WM_COMMAND",v))
    if (v)
      flags |= AC_HAS_WM_COMMAND;

  c = s ? s->GetEntry("regex") : 0;
  if (c) {
    regex = new char[strlen(c)+1];
    strcpy(regex,c);
  } else regex = 0;

  c = s ? s->GetEntry("WM_CLASS") : 0;
  if (c) {
    wmclass = new char[strlen(c)+1];
    strcpy(wmclass,c);
  } else wmclass = 0;

  if (s && s->GetIntEntry("Keep",v) && v)
    flags |= AC_KEEP;


  if (s && s->GetIntEntry("Desktop",v))
    if (v)
      flags |= AC_DESKTOP;
  c = s ? s->GetEntry("Command") : 0;
  if (c) {
    cmd = new char[strlen(c)+1];
    strcpy(cmd,c);
  }

  c = s ? s->GetEntry("Client") : 0;
  if (c) {
    lower(c);
    clientinfo = (ClientInfo *)rman->GetInfo(SE_CLIENTINFO,c);
    if (!clientinfo)
      fprintf(stderr,"%s not a valid client name\n",c);
  } else
    clientinfo = 0;

  if ((flags & AC_DESKTOP) && cmd) {
    flags &= ~AC_DESKTOP;
    flags |= AC_CLIENTDESKTOP;
  }

  s = 0;
}


Action::~Action(){
  if (cmd)
    delete [] cmd;
  if (name)
    delete [] name;
  if (regex)
    delete [] regex;
  if (wmclass)
    delete [] wmclass;
}

void Fork(char *cmd) {
  if (!cmd)
    return;
  pid_t pid = fork();
  switch (pid) {
    case 0:
      execlp("/bin/sh", "sh", "-c", cmd, NULL);
      exit(1);
    case -1:
      fprintf(stderr,"can't fork");
  }

}

void Action::Execute() {
  if (flags & AC_DESKTOP) {
    Desktop *d = new Desktop(ct->CurrentDesktop);
    d->SetClientInfo(clientinfo);
    ct->CurrentDesktop->AddDesktop(d,0);
    if (clientinfo && clientinfo->name)
      XStoreName(dpy,d->window,clientinfo->name);
  } else {
    Fork(cmd);
    if (clientinfo || (flags & (AC_HAS_WM_COMMAND | AC_KEEP | AC_CLIENTDESKTOP) || regex || wmclass)) {
      if (flags & AC_HAS_WM_COMMAND) {
        char buf[256];  // we should probably take another desktop
        ct->AddWatch(ct->CurrentDesktop,clientinfo, sscanf("%s[ &;]",cmd,buf) ? buf : cmd,regex,wmclass,flags);
      } else {
        ct->AddWatch(ct->CurrentDesktop,clientinfo,0,regex,wmclass,flags);
      }
    }
  }
}

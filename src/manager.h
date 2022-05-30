/* treewm - an X11 window manager.
 * Copyright (c) 2001-2002 Thomas Jäger <TheHunter2000 at web dot de>
 * This code is released under the terms of the GNU GPL.
 * See the included file LICENSE for details.
 */

#ifndef MANAGER_H
#define MANAGER_H

#include "global.h"

// see notes below
struct ClientList {
  Client *client;
  ClientList *next;
};


void SigHandler(int);

class Manager {
  public:
    Manager(int argc, char **argv);
    void AddDeleteClient(Client *);
    void AddReDrawClient(Client *);
    void CleanUp();
    void NextEvent();
    void HandleExpose(XExposeEvent &);
    ~Manager();
    int Run();
    void Quit();
    Client *WantFocus,*ToBeRaised;
    bool IgnoreLeave;
    struct ClientList *MustBeDeleted, *FirstReDraw, *LastReDraw;
    bool alive;
    int fifo;
    Window input;

};
#endif

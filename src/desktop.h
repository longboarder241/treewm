/* treewm - an X11 window manager.
 * Copyright (c) 2001-2002 Thomas Jäger <TheHunter2000 at web dot de>
 * This code is released under the terms of the GNU GPL.
 * See the included file LICENSE for details.
 */

#ifndef DESKTOP_H
#define DESKTOP_H

#include "global.h"
#include "client.h"


#define GOTO_RELATIVE 1
#define GOTO_MOVEFULL 2
#define GOTO_MOVEHALF 4
#define GOTO_SNAP     8

#define NI_UP    0
#define NI_DOWN  1
#define NI_LEFT  2
#define NI_RIGHT 3


#define ISDESKTOP(c) (dynamic_cast<Desktop *>(c))

class Desktop : public Client  {
  public:
    Desktop(Desktop *);
    virtual void SetClientInfo(ClientInfo *);
    virtual ~Desktop();
    virtual bool Init();
    virtual Desktop *DesktopAbove();
    virtual Client *FindPointerClient();
    virtual bool GetWindowList(MenuItem *,int &,bool,int,int,int,char *);
    virtual void RemoveClientReferences(Client *);
    void RemoveChild(Client *, int);
    virtual void SendWMDelete(int);
    virtual void SendEvent(XAnyEvent &e);
    virtual void Remove(int);
    void AutoClose();
    virtual void ReDraw();
    virtual void Raise(int);
    virtual void GrabButtons(bool=false);
    virtual void UpdateFlags(int,unsigned long);
    void UpdateTB();
    void ShowIcons(bool);
    Icon *AddIcon(Icon *);
    void NextIcon(Icon *, int);
    void RemoveIcon(Icon *);
    void GiveAway(Client *,bool);
    void Take(Client *,int,int);
    virtual void RequestDesktop(bool);
    virtual void GetFocus();
    bool Leave(int,int,bool);
    bool Goto(int,int,int);
    virtual Client *Focus();
    virtual unsigned long GetRegionMask(int,int,Client * &);
    void Tile();
    virtual void SetWMState();
    virtual void SendConfig();
    virtual void ChangeSize();
    void AutoResize();
    void Translate(int *,int *);
    Client *AddChild(Client *);
    Desktop *AddDesktop(Desktop *,Client *);
    unsigned long childflags;
    unsigned long childmask;
    Client *firstchild, *focus;
    Icon *firsticon;
    bool autoclose;
    Window StackRef[3];
    Client *LastRaised[3];
    int PosX,PosY;
    int VirtualX,VirtualY;
    RPixmap *bgpm;
	
};

#endif

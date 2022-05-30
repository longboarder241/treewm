/* treewm - an X11 window manager.
 * Copyright (c) 2001-2002 Thomas Jäger <TheHunter2000 at web dot de>
 * This code is released under the terms of the GNU GPL.
 * See the included file LICENSE for details.
 */

#ifndef ICON_H
#define ICON_H
#include "global.h"

#define IF_APPICON (1L<<1)

class Icon : public TWindow {
  public:
	Icon(Desktop *,Client *, char *);
	Icon(Desktop *,Icon *, bool);
	void Init();
	void Remove();
	virtual ~Icon();
  void GetPosition();
  void Move(); // snap and move
	void GetIconBitmap(XWMHints *, XSetWindowAttributes *);
  void GetIconWindow(XWMHints *);
  bool GetXPMFile(RPixmap *,XSetWindowAttributes *);
	void ChangeName(char *);
	void ReDraw();
	void Execute();
	bool IsIn(Window,int,int);
	Icon *next;
	Window window, iconwin;
  char *name;
	Pixmap pixmap;
	RPixmap *spm;
	
	Client *client;
	Desktop *parent;
	char *cmd;
	int x,y,w,h;
	int iconw,iconh;
	unsigned int depth;
  int MinIX,MinIY;
};

#endif

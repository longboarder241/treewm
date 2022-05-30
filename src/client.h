/* treewm - an X11 window manager.
 * Copyright (c) 2001-2002 Thomas Jäger <TheHunter2000 at web dot de>
 * This code is released under the terms of the GNU GPL.
 * See the included file LICENSE for details.
 */

#ifndef CLIENT_H
#define CLIENT_H
#include "global.h"
#include "menu.h"

// 1 - 25
#define CF_SHADED            (1L<< 1)
#define CF_IGNOREEXPOSE      (1L<< 2)
#define CF_FIXEDSIZE         (1L<< 3)
#define CF_ICONPOSKNOWN      (1L<< 4)
#define CF_GRABBED           (1L<< 5)
#define CF_NOCLOSE           (1L<< 6)
#define CF_NOMINIMIZE        (1L<< 7)
#define CF_FORCESTATIC       (1L<< 8)
#define CF_STICKY            (1L<< 9)
#define CF_HASTITLE          (1L<<10)
#define CF_HASTBENTRY        (1L<<11)
#define CF_GRABALTCLICK      (1L<<12)
#define CF_PASSFIRSTCLICK    (1L<<13)
#define CF_TITLEBARBUTTONS   (1L<<14)
#define CF_SNAP              (1L<<15)
#define CF_ABOVE             (1L<<16)
#define CF_BELOW             (1L<<17)
#define CF_RAISEONCLICK      (1L<<18)
#define CF_RAISEONENTER      (1L<<19)
#define CF_FOCUSONCLICK      (1L<<20)
#define CF_FOCUSONENTER      (1L<<21)
#define CF_AUTOSHADE         (1L<<22)
#define CF_HIDEMOUSE         (1L<<23)

#define CF_HASBEENSHAPED     (1L<<25)

// 26-31
#define DF_FIRST            (1L<<26)
#define DF_ICONSRAISED      (1L<<26)
#define DF_AUTOSCROLL       (1L<<27)
#define DF_TILE             (1L<<28)
#define DF_AUTORESIZE       (1L<<29)
#define DF_TASKBARBUTTONS   (1L<<30)
#define DF_GRABKEYBOARD     (1L<<31)

#define DEFAULTFLAGS (CF_HASTBENTRY | CF_HASTITLE | CF_GRABALTCLICK | CF_PASSFIRSTCLICK | \
                      CF_SNAP | CF_TITLEBARBUTTONS | CF_FOCUSONENTER | DF_TASKBARBUTTONS)

#define DEFAULTMASK (CF_IGNOREEXPOSE | CF_FIXEDSIZE | CF_ICONPOSKNOWN | CF_GRABBED | \
                     CF_FORCESTATIC | CF_HASBEENSHAPED | TF_ISICON | DF_ICONSRAISED)

#define CF_ALL 0x00FFFFFE
#define DF_ALL 0xFF000000

#define FLAG_NONE         0
#define FLAG_UPDATE       1
#define FLAG_SET          2
#define FLAG_UNSET        3
#define FLAG_TOGGLE       4
#define FLAG_REMOVE       5
#define FLAG_DSET         6
#define FLAG_DUNSET       7
#define FLAG_DTOGGLE      8
#define FLAG_DREMOVE      9
#define FLAG_CSET    10
#define FLAG_CUNSET  11
#define FLAG_CTOGGLE 12
#define FLAG_CREMOVE 13

#define FLAG_CMIN FLAG_CSET
#define FLAG_CMAX FLAG_CREMOVE

#define R_WITHDRAW 0
#define R_REMAP 1
#define R_RESTART 2

#define SPACE 1
#define BUTTONWIDTH THeight


// modes for raise

#define R_PARENT 1
#define R_MOVEMOUSE 2
#define R_INDIRECT 4
#define R_GOTO 8
#define R_MAP 16

#define L_PARENT 1
#define L_BOTTOM 2

#define snap 6

#define S_LEFT 1
#define S_RIGHT 2

#define S_TOP 4
#define S_BOTTOM 8

// Defines for Maximize
#define MAX_UNMAX 0
#define MAX_HALF 1
#define MAX_FULL 2
#define MAX_REMAX 3
#define MAX_FULLSCREEN 4
#define MAX_HORI 5
#define MAX_VERTHALF 6
#define MAX_VERTFULL 7

// Defines for SendWMDelete
#define DEL_NORMAL  0
#define DEL_FORCE   1
#define DEL_RELEASE 2

// Defines for MH->type
#define T_B1     1
#define T_B2     2
#define T_B3     3
#define T_DC     8  // Doubleclick
#define T_SDC    16 // Strange doubleclick
#define T_BAR    32
#define T_F1     64
#define T_F2     96
#define T_SHIFT 128

#define T_BUTTON (T_BAR|T_F1|T_F2)
#define T_MOUSEBUTTON (T_B1|T_B2|T_B3)


class Client : public TWindow {
  public:
    Client(Desktop *, Window = 0);
    virtual void SetClientInfo(ClientInfo *);
    virtual bool Init();
    void GetMWMOptions();
    virtual ~Client();
    virtual Client* FindPointerClient();
    virtual bool GetWindowList(MenuItem *,int &,bool,int,int,int,char *);
    virtual void RemoveClientReferences(Client *);
    virtual Desktop *DesktopAbove();
    void ChangeName();
    void GetFlags();
    virtual void UpdateFlags(int, unsigned long);
    void UpdateName(bool);
    void ReDrawTBEntry(Client *,bool=true);
    virtual void SendWMDelete(int);
    virtual void SendEvent(XAnyEvent &e);
    virtual void RequestDesktop(bool);
    void Maximize(int);
    void Shade();
    virtual unsigned long GetRegionMask(int,int,Client * &);
    virtual void SetWMState();
    virtual void GrabButtons(bool = false);
    void GiveFocus();
    virtual void GetFocus();
    virtual Client *Focus();
    Client* Next(bool);
    Client* Prev(bool);
    void MoveRight();
    void MoveLeft();
    void MoveUp();
    void MoveDown();
    void Stacking(bool);
    int GetStacking();
    virtual void Raise(int);
    void Scroll(bool);
    void Lower(unsigned int);
    void LowerBelow(Client *);
    void Map();
    void Unmap(bool);
    long GetWMState();
    virtual void SendConfig();
    void Configure(XConfigureRequestEvent);
    void InitPosition();
    void Gravitate(int);
    void Reparent();
    virtual void Remove(int);
    void Hide();
    void ToggleHasTitle();
    virtual void UpdateTB();
    virtual void ReDraw();
    void DrawOutline();
    void MoveResize(int, bool, int, int, bool = false);
    void Snap(int);
    void RecalcSweep(int);
    virtual void ChangeSize();
    void ChangePos(bool);
    void Clear();
#ifdef SHAPE
    void MakeHole(bool);
    void SetShape();
#endif
#ifdef DEBUG
    const char *ShowGrav();
    const char *ShowState();
    void dump();
#endif
    unsigned long mask;
    unsigned long parentmask; // why here????
    Client *next;
    Desktop *parent,*target;
    char	*name, *wmname;
    bool mapped; // Is the window mapped?
    bool visible; // Is the window visible?
    int THeight;
    Window	window, frame, title;

    Client *trans,*transfor;
//  protected:
    Sceme *Sc;
//    ClientInfo *ci; doesn't make sence
    int TBx, TBy, TBEx, TBEy;

    int MaxX,MaxY,MaxW,MaxH;

    XSizeHints	*size;
    Colormap cmap;
    int	x, y, width, height;
    int x_root, y_root;
    int IconX,IconY;

    Icon *icon;
    RPixmap *iconpm;
    int ignoreunmap;

};


void GetMousePosition(Window , int &, int &);
void FetchName(Window , char **);

#define setmouse(w, x, y) XWarpPointer(dpy, None, w, 0, 0, 0, 0, x, y)
#define grabp(w, mask, curs) (XGrabPointer(dpy, w, False, mask, \
    GrabModeAsync, GrabModeAsync, None, curs, CurrentTime) == GrabSuccess)

#define DWidth DisplayWidth(dpy,screen)
#define DHeight DisplayHeight(dpy,screen)
#ifdef VIDMODE
#define VWidth ct->ctVWidth
#define VHeight ct->ctVHeight
#define VX ct->ctVX
#define VY ct->ctVY
#else
#define VWidth DWidth
#define VHeight DHeight
#define VX 0
#define VY 0
#endif

#endif

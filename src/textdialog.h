/* treewm - an X11 window manager.
 * Copyright (c) 2001-2003 Thomas Jäger <TheHunter2000 at web dot de>
 * This code is released under the terms of the GNU GPL.
 * See the included file LICENSE for details.
 */

#ifndef TEXTDIALOG_H
#define TEXTDIALOG_H

#include "global.h"

#define DIALOGSPACE 4
#define MAXLENGTH 256
#define HISTSIZE 16

class TextDialog : public TWindow  {
  public:
    TextDialog(Sceme *);
    ~TextDialog();
    void ReDraw();
    void AddHistory();
    void ChangeHist(int);
    char *Key(XKeyEvent &);
    void SetCursor(int);
    void SetLine(char *);
    void Complete();
    void Paste();
    Window window;
    Sceme *Sc;
    int height;
    char line[MAXLENGTH];
    static char *hist[HISTSIZE];
    static int histpos;
    int histref;
    int cur,length;
    XComposeStatus cstatus;
};

#endif

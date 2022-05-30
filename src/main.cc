/* treewm - an X11 window manager.
 * Copyright (c) 2001-2002 Thomas Jäger <TheHunter2000 at web dot de>
 * This code is released under the terms of the GNU GPL.
 * See the included file LICENSE for details.
 */

#include "manager.h"
#include <ctype.h>
#include <X11/Xlocale.h>

Display *dpy;
int screen;
Window root;

#ifdef SHAPE
Bool shape;
int shape_event;
#endif

void lower(char *str) {
  for (int i=0;str[i]!='\0';++i)
    str[i] = tolower(str[i]);
}

#ifdef DEBUG
void err(const char *fmt, ...)
{
    va_list argp;

    fprintf(stderr, "debug: ");
    va_start(argp, fmt);
    vfprintf(stderr, fmt, argp);
    va_end(argp);
    fprintf(stderr, "\n");
}

bool error;
#endif

Info::~Info() {
}

int main(int argc, char **argv)
{
#ifdef DEBUG
  error = false;
#endif
  setlocale(LC_CTYPE, "");
  /*man =*/ new Manager(argc, argv);
  int r=man->Run();
  delete man;
#ifdef DEBUG
  err("Exit...");
#endif
  return r;
}

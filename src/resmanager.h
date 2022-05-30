/* treewm - an X11 window manager.
 * Copyright (c) 2001-2002 Thomas Jäger <TheHunter2000 at web dot de>
 * This code is released under the terms of the GNU GPL.
 * See the included file LICENSE for details.
 * 
 * changed<05.02.2003> by Rudolf Polzer: replaced hash_map by map (for gcc 3.2)
 */

#ifndef RESMANAGER_H
#define RESMANAGER_H

#include "global.h"
#include "client.h"
#include <map>
#include <string.h>

struct ltstr {
  bool operator() (const char* s1, const char* s2) const  {
    return strcmp(s1, s2) < 0;
  }
};

typedef map<const char *,Info *, ltstr> InfoList;
typedef InfoList::iterator InfoListIter;

typedef map<const char *,unsigned long, ltstr> FlagMap;
typedef FlagMap::iterator FlagMapIter;

struct Entry {
  char *tag, *value;
  Entry *next;
};

struct FlagInfo {
  char *str;
  unsigned long flag;
};

// strings MUST be lower case here
#define CLIENTFLAGNUM 25
const FlagInfo ClientFlagArray[CLIENTFLAGNUM] = {
  {"shaded",CF_SHADED},
  {"autoshade",CF_AUTOSHADE},
  {"fixedsize",CF_FIXEDSIZE},
  {"noclose",CF_NOCLOSE},
  {"nominimize",CF_NOMINIMIZE},
  {"sticky",CF_STICKY},
  {"hastitle",CF_HASTITLE},
  {"hastbentry",CF_HASTBENTRY},
  {"grabaltclick", CF_GRABALTCLICK},
  {"passfirstclick", CF_PASSFIRSTCLICK},
  {"titlebarbuttons",CF_TITLEBARBUTTONS},
  {"snap",CF_SNAP},
  {"above",CF_ABOVE},
  {"below",CF_BELOW},
  {"hidemouse",CF_HIDEMOUSE},
  {"raiseonclick", CF_RAISEONCLICK},
  {"raiseonenter", CF_RAISEONENTER},
  {"focusonclick", CF_FOCUSONCLICK},
  {"focusonenter", CF_FOCUSONENTER},
  {"autoscroll", DF_AUTOSCROLL},
  {"tile", DF_TILE},
  {"autoresize",DF_AUTORESIZE},
  {"taskbarbuttons",DF_TASKBARBUTTONS},
  {"buttons",CF_TITLEBARBUTTONS | DF_TASKBARBUTTONS},
  {"grabkeyboard",DF_GRABKEYBOARD}
};

#define FLAGACTNUM 12

const FlagInfo FlagActArray[FLAGACTNUM] = {
  {"set",FLAG_SET},
  {"unset", FLAG_UNSET},
  {"toggle", FLAG_TOGGLE},
  {"remove", FLAG_REMOVE},
  {"dset", FLAG_DSET},
  {"dunset", FLAG_DUNSET},
  {"dtoggle", FLAG_DTOGGLE},
  {"dremove", FLAG_DREMOVE},
  {"cset", FLAG_CSET},
  {"cunset", FLAG_CUNSET},
  {"ctoggle", FLAG_CTOGGLE},
  {"cremove", FLAG_CREMOVE}
};



#define SE_SCEME      0
#define SE_ACTION     1
#define SE_CLIENTINFO 2
#define SE_MENUINFO   3
#define SE_NUM        4


class Section {
  public:
    ~Section();
    void SetEntry(char *,char *);
    char *GetEntry(char *);
    void IncludeSection(char *);
    bool GetIntEntry(char *, int &);
    int type;
    char *name;
    Entry *fe;
    Section *next;
};

class RPixmap {
  public:
    ~RPixmap();
    Pixmap GetPixmap();
    void FreePixmap();
    int n,w,h;
    char *path;
    Pixmap pm;
    RPixmap *next;

};

class ResManager {
  public:
    ResManager(char *);
    ~ResManager();
    Section *GotoSection(char *,char *);
    void DeleteSections();
    Info *GetInfo(int,char *);
    void CreateScemes();
    void LoadCFGFile();
    unsigned long GetFlag(char *);
    int GetFlagAct(char *);
    RPixmap *GetPixmap(const char *);
    RPixmap *pixmaps;
    InfoList *infos[SE_NUM];
    Section *sections;
    char *config;
    FlagMap ClientFlags,FlagActFlags;
};

#endif

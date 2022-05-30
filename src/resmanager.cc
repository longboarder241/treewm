/* treewm - an X11 window manager.
 * Copyright (c) 2001-2002 Thomas Jäger <TheHunter2000 at web dot de>
 * This code is released under the terms of the GNU GPL.
 * See the included file LICENSE for details.
 */

#include "resmanager.h"
#include "sceme.h"
#include "clientinfo.h"
#include "menuinfo.h"
#include "action.h"
#include "desktop.h"
#include <X11/xpm.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h> // for isspace


ResManager *rman;


ResManager::ResManager(char *Config) : ClientFlags() {
  for (int i=0;i!=SE_NUM;++i)
    infos[i] = new InfoList;
  rman = this; // shit
  config = Config;
  pixmaps = 0;

  sections = new Section;
  sections->type = SE_SCEME; // We definitely need a default sceme
  sections->name = 0;
  sections->fe = 0;
  sections->next = 0;

  LoadCFGFile();
  CreateScemes();
  DeleteSections();
  for (int i = 0; i!=CLIENTFLAGNUM; ++i)
    ClientFlags[ClientFlagArray[i].str] = ClientFlagArray[i].flag;
  for (int i = 0; i!=FLAGACTNUM; ++i)
    FlagActFlags[FlagActArray[i].str] = FlagActArray[i].flag;

}

int ResManager::GetFlagAct(char *str) {
  FlagMapIter i = FlagActFlags.find(str);
  if (i == FlagActFlags.end())
    return 0;
  return i->second;
}

unsigned long ResManager::GetFlag(char *str) {
  FlagMapIter i = ClientFlags.find(str);
  if (i == ClientFlags.end())
    return 0;
  return i->second;

}


ResManager::~ResManager() {
  for (int t=0;t!=SE_NUM;++t) {
    int n = infos[t]->size();
    char *a[n];
    int k = 0;
    for (InfoListIter i = infos[t]->begin();i != infos[t]->end();++i) {
      a[k] = (char *)i->first;
      delete i->second;
      ++k;
    }
    delete infos[t];
    for (k=0;k!=n;++k)
      free(a[k]);
  }

  RPixmap *ps;
  for (;pixmaps;pixmaps=ps) {
    ps = pixmaps->next;
    delete pixmaps;
  }

}



Info *ResManager::GetInfo(int type, char *name) {
  InfoListIter i = infos[type]->find(name);
  if (i == infos[type]->end())
    return 0;
  return i->second;
}

void ResManager::CreateScemes() {
  int t;
  Info* info = 0;
  for (Section *s=sections;s;s=s->next) {
    t = s->type;
    switch (t) {
      case SE_CLIENTINFO:
        info = new ClientInfo(s);
        break;
      case SE_SCEME:
        info = new Sceme(s);
        break;
      case SE_ACTION:
        info = new Action(s);
        break;
      case SE_MENUINFO:
        info = new MenuInfo(s);
        break;
      default:
        exit(1);
    }
    char *c =strdup(s->name ? s->name : "");
    (*infos[t])[c] = info;
  }

  for (int t=0;t!=SE_NUM;++t) {
    for (InfoListIter i = infos[t]->begin();i != infos[t]->end();++i) {
      i->second->Init();
    }
  }

  // Dirty hack
  for (InfoListIter i = infos[SE_MENUINFO]->begin();i != infos[SE_MENUINFO]->end();++i) {
    MenuInfo *mi = (MenuInfo *)i->second;
    for (int j=0; j!=mi->n; ++j) {
      MenuItem &mitem = mi->menu[j];
      if (mitem.key[0] == '$') {
        MenuInfo *mi2 = (MenuInfo *)rman->GetInfo(SE_MENUINFO,mitem.key+1);
        if (mi2) {
          mitem.submenu = new SubMenuInfo;
          mitem.submenu->menu = mi2->menu;
          mitem.submenu->num  = mi2->n;
          free(mitem.key);
          mitem.key = strdup("");
        }
      }
    }
  }



}


RPixmap *ResManager::GetPixmap(const char *name) {
  RPixmap *last=0;
  for (RPixmap *pm = pixmaps;pm;pm=pm->next) {
    if (!strcmp(pm->path,name))
      return pm;
    last = pm;
  }
  if (last) {
    last->next = new RPixmap();
    last = last->next;
  } else {
    pixmaps = new RPixmap();
    last = pixmaps;
  }
  last->n = 0;
  last->path = new char[strlen(name)+1];
  strcpy(last->path,name);
  last->pm = 0;
  last->next = 0;

  return last;
}

Pixmap RPixmap::GetPixmap() {
  if (!n++) {
    Pixmap dummy;
    char buf[256];
    if (path[0] == '/') {
      buf[0] = '\0';
    } else {
      strcpy(buf,PIXMAPS);
      strncat(buf,path,255);
    }
    int r;
    XWindowAttributes root_attr;
    XpmAttributes xpm_attributes;
    XGetWindowAttributes(dpy,root,&root_attr);
    xpm_attributes.colormap = root_attr.colormap;
    xpm_attributes.closeness = 40000; /* Allow for "similar" colors */
    xpm_attributes.valuemask = XpmSize | /*XpmReturnPixels |*/ // bullshit
                               XpmColormap | XpmCloseness;
    r = XpmReadFileToPixmap(dpy, root, buf[0] ? buf : path, &pm, &dummy,&xpm_attributes);
    if (r!=XpmSuccess) {
      n = 0;
      return 0;
    }
    if (dummy)
      XFreePixmap(dpy,dummy);
    w = xpm_attributes.width;
    h = xpm_attributes.height;
  }
  return pm;
}

void RPixmap::FreePixmap() {
  if (--n > 0)
    return;
  n = 0;
  if (pm)
    XFreePixmap(dpy,pm);
}

RPixmap::~RPixmap() {
  if (pm && n)
    XFreePixmap(dpy,pm);
  if (path)
    delete [] path;
}


Section *ResManager::GotoSection(char *type,char *name) {
  int newtype = -1;
  if (!strcasecmp(type, "client"))
    newtype = SE_CLIENTINFO;
  if (!strcasecmp(type, "sceme"))
    newtype = SE_SCEME;
  if (!strcasecmp(type, "action"))
    newtype = SE_ACTION;
  if (!strcasecmp(type, "menu"))
    newtype = SE_MENUINFO;
  if (newtype == -1)
    return 0;
  Section *s,*last=0;
  for (s=sections;s;s=s->next) {
    if (newtype == s->type &&
        ((!name && !s->name) ||
         (name && s->name && !strcasecmp(name,s->name)) )) {
      return s;
    }
    last = s;
  }
  s = new Section;
  s->type = newtype;
  s->fe = 0;
  s->next = 0;
  if (name) {
    s->name = new char[strlen(name)+1];
    strcpy(s->name,name);
    lower(s->name);
  } else {
    s->name = 0;
  }
  if (last)
    last->next=s; else
    sections = s;
  return s;

}


void ResManager::LoadCFGFile() {
// This is mainly stolen from xawtv

  Section *s=0;
  char filename[256];
  if (config)
    strncpy(filename,config,255); else
    snprintf(filename,255,"%s/%s",getenv("HOME"),".treewm");
  filename[255] = '\0';

  char line[256],tag[64],value[192];
  FILE *fp;
  int nr,k;

  if (NULL == (fp = fopen(filename,"r"))) {
    fprintf(stderr,"Warning: no config file (~/.treewm) found\n");
    return;
  }

  nr = 0;
  while (NULL != fgets(line,255,fp)) {
    ++nr;
    if (line[0] == '\n' || line[0] == '#' || line[0] == '%')
      continue;
    for (k=1;k!=255;++k)
      if (line [k] == '\0' || line[k] == '\n' || line[k] == '#' || line[k] == '%') {
        line[k] = '\0';
        break;
      }
    for (--k;k;--k)
      if (!isspace(line[k])) {
        line[k+1] = '\0';
        break;
      }
    if (!k)
      continue;

    k = sscanf(line,"[%63[^] ] %191[^]]]",tag,value);

    if (k) {
      /* [type name] */
      s = GotoSection(tag,(k == 2) ? value : 0);
      if (!s) {
        fprintf(stderr,"%s,%d: not a valid section specifier\n",filename,nr);
        continue;
      }
    } else {
      k = sscanf(line," %63[^= ] = %191[^\n]",tag,value);
      if (k) {
      /* tag = value */
        if (s) {
          if (strcasecmp(tag,"include")) {
            if (k==2)
              s->SetEntry(tag,value);
          } else {
            s->IncludeSection((k == 2) ? value : 0);
          }
        } else {
          fprintf(stderr,"%s:%d: error: no section\n",filename,nr);
        }
      } else {
        /* Huh ? */
        fprintf(stderr,"%s:%d: syntax error\n",filename,nr);
      }
    }
  }
  fclose(fp);
  return;
}

void ResManager::DeleteSections() {
  Section *saves;
  for (Section *s=sections;s;s=saves) {
    saves = s->next;
    delete s;
  }
}


void Section::SetEntry(char *tag,char *value) {
  Entry *e,*last=0;
  for (e=fe;e;e=e->next) {
    if (!strcasecmp(tag,e->tag)) {
      if (e->value)
        delete [] e->value;
      e->value = new char[strlen(value)+1];
      strcpy(e->value,value);
      return;
    }
    last = e;
  }
  e = new Entry;
  e->tag = new char[strlen(tag)+1];
  strcpy(e->tag,tag);
  e->value = new char[strlen(value)+1];
  e->next = 0;
  strcpy(e->value,value);
  if (last)
    last->next=e; else
    fe = e;
  return;
}

char *Section::GetEntry(char *tag) {
  for (Entry *e=fe;e;e=e->next)
    if (!strcasecmp(tag,e->tag))
      return e->value;
  return 0;
}

bool Section::GetIntEntry(char *tag,int &ret) {
  char *str = GetEntry(tag);
  if (!str)
    return false;
  ret = atoi(str);
  if (!ret) {
    if (!strcasecmp(str,"on") || !strcasecmp(str,"true") || !strcasecmp(str,"yes"))
      ret = 1;
  }
  return true;
}

void Section::IncludeSection(char *name) {
  Section *s;
  for (s = rman->sections;;s=s->next) {
    if (type == s->type &&
        ((!name && !s->name) ||
         (name && s->name && !strcasecmp(name,s->name)) ))
      break;
    if (!s)
      return;
  }

  for (Entry *e=s->fe;e;e=e->next)
    SetEntry(e->tag,e->value);

}


Section::~Section() {
  Entry *savee;
  if (name)
    delete [] name;
  for (Entry *e=fe;e;e=savee) {
    savee = e->next;
    if (e->tag)
      delete [] e->tag;
    if (e->value)
      delete [] e->value;
    delete e;
  }
}

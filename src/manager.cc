/* treewm - an X11 window manager.
 * Copyright (c) 2001-2002 Thomas Jäger <TheHunter2000 at web dot de>
 * This code is released under the terms of the GNU GPL.
 * See the included file LICENSE for details.
 */

#include "desktop.h"
#include "manager.h"
#include "icon.h"
#include "uehandler.h"
#include "resmanager.h"
#include "clienttree.h"
#include "sceme.h"
#include "menu.h"
#include "MwmUtil.h"
#include "textdialog.h"
#include "action.h"
#include <unistd.h>
#include <X11/Xmd.h>
#include <X11/Xatom.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <X11/keysym.h>

Manager *man;

Atom wm_state, wm_change_state, wm_protos, wm_delete, wm_cmapwins, wm_client_leader,
     mwm_hints;




int handle_xerror(Display *dpy, XErrorEvent *e) {
  Client *c = ct ? ct->FindClient(e->resourceid, 0) : 0;

  if (e->error_code == BadAccess && e->resourceid == root) {
    fprintf(stderr,"root window unavailible (maybe another wm is running?)\n");
    exit(1);
  } else {
    char msg[80], req[80], number[80];
    XGetErrorText(dpy, e->error_code, msg, sizeof msg - 1);
    msg[79]='\0';
    snprintf(number, sizeof number - 1, "%d", e->request_code);
    number[79] = '\0';
    XGetErrorDatabaseText(dpy, "XRequest", number, "", req, sizeof(req) - 1);
    req[79]='\0';
    if (req[0] == '\0')
      sprintf(req, "<request-code-%d>", e->request_code);
    fprintf(stderr,"XERROR: %s(0x%x): %s\n", req, (unsigned int)e->resourceid, msg);

  }
#ifdef DEBUG
  fprintf(stderr,"\7");
#endif


//  if (c && c->parent)
//    c->parent->RemoveChild(c,R_WITHDRAW);
/*
 *  Here, a very interesting bug happens. If we call an X function during the
 *  processing of an event and an error occurs and we immediately delete the window
 *  we're working with, we cause a lot of shit
 *  therefore, we have to be _very_ careful

 *  it's just because of these crappy programs that change their titles before they
 *  kill themselves
*/
  if (e->error_code == BadWindow && c && c->parent)
    man->AddDeleteClient(c);
  return 0;
}

void Manager::AddDeleteClient(Client *c) {
  if (man->MustBeDeleted) {
    ClientList *cl = man->MustBeDeleted;
    for (;;cl=cl->next) {
      if (cl->client == c)
        break;
      if (!cl->next) {
        cl->next = new ClientList;
        cl = cl->next;
        cl->next = 0;
        cl->client = c;
        break;
      }
    }
  } else {
    man->MustBeDeleted = new ClientList;
    man->MustBeDeleted->client = c;
    man->MustBeDeleted->next = 0;
  }
}

void SigHandler(int signal) {
  switch (signal) {
    case SIGHUP:
//      man->Restart();
//      break;
    case SIGINT:
    case SIGTERM:
      man->Quit();
      exit(0);
      break;
    case SIGCHLD:
      wait(0);
      break;
  }
}


Manager::Manager(int argc, char **argv) {
  man = this;

  XSetWindowAttributes sattr;
  struct sigaction act;

  ct = NULL; // if an error happens, this value is used

  act.sa_handler = SigHandler;
  act.sa_flags = 0;
  sigaction(SIGTERM, &act, NULL);
  sigaction(SIGINT, &act, NULL);
  sigaction(SIGHUP, &act, NULL);
  sigaction(SIGCHLD, &act, NULL);

  int display = 0;
  int config = 0;
  for (int i=1;i!=argc;++i) {
    if (!strcmp(argv[i],"-display") && ++i != argc) {
      display = i;
      continue;
    }
    if (!strcmp(argv[i],"-f") && ++i != argc) {
      config = i;
      continue;
    }
    printf("usage: %s [-display displayname] [-f configfile]\n",argv[0]);
    exit(1);

  }
  dpy = XOpenDisplay(display ? argv[display] : 0);
  if (dpy) {
// thx to Volker Grabsch
    if (display)
      setenv("DISPLAY",argv[display],1);
  }
  else {
    fprintf(stderr,"can't open display %s!\n",display ? argv[display]: 0);
    exit(1);
  }


  XSetErrorHandler(handle_xerror);

  char filename[256];
  snprintf(filename,255,"%s/%s",getenv("HOME"),".cmdtreewm");
  filename[255] = '\0';
  mkfifo(filename,0600);
  fifo = open(filename,O_RDWR | O_NONBLOCK);
  if (!(fifo + 1))
    fprintf(stderr,"warning: couldn't create fifo (~/.cmdtreewm)");

  screen = DefaultScreen(dpy);
  root = RootWindow(dpy, screen);


  wm_state = XInternAtom(dpy, "WM_STATE", false);
  wm_change_state = XInternAtom(dpy, "WM_CHANGE_STATE", false);
  wm_protos = XInternAtom(dpy, "WM_PROTOCOLS", false);
  wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", false);
  wm_cmapwins = XInternAtom(dpy, "WM_COLORMAP_WINDOWS", false);
  wm_client_leader = XInternAtom(dpy, "WM_CLIENT_LEADER", false);
  mwm_hints = XInternAtom(dpy, _XA_MWM_HINTS, false);
#ifdef SHAPE
  int dummy;
  shape = XShapeQueryExtension(dpy, &shape_event, &dummy);
#endif

  sattr.event_mask = ChildMask|ColormapChangeMask|ButtonMask;
  XChangeWindowAttributes(dpy, root, CWEventMask, &sattr);

	
	sattr.override_redirect = 1;
	sattr.event_mask = KeyPressMask | KeyReleaseMask;
	input = XCreateWindow(dpy, root, 0, 0, 1, 1, 0,
		CopyFromParent, InputOnly, CopyFromParent, CWEventMask | CWOverrideRedirect, &sattr);
	XMapWindow(dpy, input);

//  XSelectInput(dpy,root,sattr.event_mask);

  LastReDraw = 0;
  FirstReDraw  = 0;

// Scan Windows now
  /*rman = */ new ResManager(config ? argv[config] : 0);
  /*UEH = */  new UEHandler();
  /*ct = */   new ClientTree();
  for (InfoListIter il = rman->infos[SE_ACTION]->begin(); il!=rman->infos[SE_ACTION]->end(); ++il)
    if (((Action *)il->second)->flags & AC_AUTOSTART) {
      ((Action *)il->second)->Execute();
    }

  unsigned int nwins, i;
  Window dummyw1, dummyw2, *wins;
  XWindowAttributes attr;
  XQueryTree(dpy, root, &dummyw1, &dummyw2, &wins, &nwins);
  for (i = 0; i != nwins; ++i) {
    XGetWindowAttributes(dpy, wins[i], &attr);
    if (!attr.override_redirect && attr.map_state == IsViewable)
      ct->AddWindow(wins[i]);
  }
  XFree(wins);


  ct->RootDesktop->GetFocus();
  WantFocus = 0;
  ToBeRaised = 0;

}

Manager::~Manager() {

}

void Manager::Quit() {
  delete ct;
  delete UEH;
  delete rman;
  XInstallColormap(dpy, DefaultColormap(dpy, screen));
  XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);

  XCloseDisplay(dpy);

}


#ifdef DEBUG

#define SHOW_EV(name, memb) \
    case name: s = #name; w = e.memb.window; break;

void ShowEvent(XEvent e) {
    char sbuf[20],*s=sbuf, buf[20];
    sbuf[0]='\0';
    Window w=0;
    Client *c=NULL;
    switch (e.type) {
        SHOW_EV(ButtonPress, xbutton)
        SHOW_EV(ButtonRelease, xbutton)
        SHOW_EV(ClientMessage, xclient)
        SHOW_EV(ColormapNotify, xcolormap)
        SHOW_EV(ConfigureNotify, xconfigure)
        SHOW_EV(ConfigureRequest, xconfigurerequest)
        SHOW_EV(CreateNotify, xcreatewindow)
        SHOW_EV(DestroyNotify, xdestroywindow)
        SHOW_EV(EnterNotify, xcrossing)
        SHOW_EV(Expose, xexpose)
        SHOW_EV(LeaveNotify, xcrossing)
        SHOW_EV(MapNotify, xmap)
        SHOW_EV(MapRequest, xmaprequest)
        SHOW_EV(MappingNotify, xmapping)
        SHOW_EV(MotionNotify, xmotion)
        SHOW_EV(PropertyNotify, xproperty)
        SHOW_EV(ReparentNotify, xreparent)
        SHOW_EV(ResizeRequest, xresizerequest)
        SHOW_EV(UnmapNotify, xunmap)
        SHOW_EV(KeyPress, xkey)
        SHOW_EV(KeyRelease, xkey)
        default:
#ifdef SHAPE
          if (shape && e.type == shape_event) {
            strcpy(s,"ShapeNotify"); w = ((XShapeEvent *)&e)->window;
          } else {
#endif
            snprintf(s, 19,"unknown event %i",e.type);
            w = None;
#ifdef SHAPE
          }
#endif
          break;
    }
    Icon *i = 0;
    c = ct->FindClient(w, &i);
    if (i)
      c = i->parent;
    strncpy(buf, c ? (c->name ? c->name : "") : "(none)", sizeof buf-1);
    buf[sizeof buf-1]='\0';
    err("%#-10lx: %-20s: %s", w, buf, s);

}

#endif

void Manager::AddReDrawClient(Client *c) {
  if (LastReDraw) {
    LastReDraw->next = new ClientList;
    LastReDraw = LastReDraw->next;
  } else {
    FirstReDraw = new ClientList;
    LastReDraw = FirstReDraw;
  }
  LastReDraw->next = 0;
  LastReDraw->client = c;
}

void Manager::CleanUp() {
// Now delete the windows Xerror found
  for (;MustBeDeleted;) {
    ClientList *cl = MustBeDeleted;
    if (MustBeDeleted->client)
      MustBeDeleted->client->parent->RemoveChild(MustBeDeleted->client,R_WITHDRAW);
    MustBeDeleted = MustBeDeleted->next;
    delete cl;
  }

  if (ToBeRaised && !XPending(dpy)) {
#ifdef DEBUG
    err("Raising Window");
#endif
    ToBeRaised->Raise(R_PARENT|R_INDIRECT);
    ToBeRaised = 0;
  }
  if (WantFocus && !XPending(dpy)) {
#ifdef DEBUG
    err("Giving Focus to Window");
#endif
    WantFocus->GetFocus();
    WantFocus = 0;
  }
  if (!ct->Focus && !ct->MaxWindow && !XPending(dpy)) {
#ifdef DEBUG
    err("Focus Lost");
#endif
    Client *c = ct->FindPointerClient();
//      c->Raise(R_PARENT|R_INDIRECT);
    c->GetFocus();
  }

  if (FirstReDraw && !XPending(dpy)) {
    ClientList *i;
    for (;FirstReDraw;FirstReDraw = i) {
      i = FirstReDraw->next;
      if (FirstReDraw->client) {
        if (FirstReDraw->client->visible)
          FirstReDraw->client->ReDraw();
        FirstReDraw->client->flags &= ~CF_IGNOREEXPOSE;
      }
      delete FirstReDraw;
    }
    LastReDraw = 0;

  }

  if (!XPending(dpy)) { // No more events
    if (IgnoreLeave && XSync(dpy,false) && !XPending(dpy)) // avoid annoying race condition
      IgnoreLeave = false;
  }
}


void Manager::NextEvent() {
  fd_set read_fds;
  while (!XPending(dpy) && fifo+1) {
#define BUFSIZE 255
    char buffer[BUFSIZE+1];
    int length = read(fifo, buffer, BUFSIZE);
    while (length > 0) {
      int i1=0,i2=0;
      while (i2 != length) {
        if (buffer[i2] == '\n') {
          buffer[i2++] = '\0';
          UEH->ExecCommands(buffer+i1);
          i1 = i2;
        } else {
          ++i2;
        }
      }
      if (length < BUFSIZE)
        break;
      if (i1) {
        i1 = i2 - i1;
        int i = i1;
        while (i1)
          buffer[--i1] = buffer[--i2];
        length = read(fifo,buffer + i,BUFSIZE - i) + i;
        if (length == i)
          break;
      } else {
        while (read(fifo,buffer,BUFSIZE) > 0);
        break;
      }
    }
    if (XPending(dpy))
      break;
    CleanUp();
    if (XPending(dpy))
      break;
    FD_ZERO(&read_fds);
    int fd1 = ConnectionNumber(dpy);
    FD_SET(fd1, &read_fds);
    FD_SET(fifo,&read_fds);
    select(1 + ((fd1<fifo) ? fifo : fd1), &read_fds, 0, 0, 0);
  }

}

void Manager::HandleExpose(XExposeEvent &e) {
  if (e.count)
    return;
  Icon *i=0;
  Client *c = ct->FindClient(e.window, &i);
  if (i) {
    i->ReDraw();
    return;
  }
  if (c) {
    AddReDrawClient(c);
    return;
  }
  if (UEH->menu) {
    Menu *m = UEH->menu;
    while (m) {
      if (e.window == m->window) {
        m->ReDraw();
        return;
      }
      m = m->submenu;
    }
  }
  if (UEH->dialog && e.window == UEH->dialog->window) {
    UEH->dialog->ReDraw();
    return;
  }

}


int Manager::Run() {
  XEvent ev;
#ifdef DEBUG
  err("Entering event loop");
#endif
  MustBeDeleted = 0;
  IgnoreLeave = false;
  alive = true;
  for (;alive;) {
    NextEvent();
    XNextEvent(dpy, &ev);
#ifdef DEBUG
    ShowEvent(ev);
#endif
    Client *c;
    switch (ev.type) {
      case KeyRelease:
      case KeyPress:
        UEH->Key(ev.xkey);
        break;
      case ButtonPress:
        UEH->Press(ev.xbutton);
        break;
      case ButtonRelease:
        UEH->Release(ev.xbutton);
        break;
      case MotionNotify:
        UEH->Motion(ev.xmotion);
        break;
      case ConfigureRequest:
        c = ct->FindClient(ev.xconfigurerequest.window, 0);
        if (c) {
          c->Configure(ev.xconfigurerequest);
        } else {
          XWindowChanges wc;
          wc.x = ev.xconfigurerequest.x;
          wc.y = ev.xconfigurerequest.y;
          wc.width = ev.xconfigurerequest.width;
          wc.height = ev.xconfigurerequest.height;
          wc.sibling = ev.xconfigurerequest.above;
          wc.stack_mode = ev.xconfigurerequest.detail;
          XConfigureWindow(dpy, ev.xconfigurerequest.window, ev.xconfigurerequest.value_mask, &wc);
        }
        break;
      case MapRequest:
        c = ct->FindClient(ev.xmaprequest.window, 0);
        if (c) {
          if (!c->visible && c->parent) {
            c->Map();
            c->Raise(0);
          }
        } else {
          ct->AddWindow(ev.xmaprequest.window);
        }
        break;
      case MapNotify:
        c = ct->FindClient(ev.xmap.window,0);
        if (c) {
          if (ev.xunmap.window != c->window)
            break;
          c->Map();
        }
        break;
      case UnmapNotify:
        c = ct->FindClient(ev.xunmap.window,0);
        if (c) {
          if (ev.xunmap.window != c->window)
            break;
          c->Unmap(true);
        }
        break;
      case DestroyNotify:
        c = ct->FindClient(ev.xdestroywindow.window, 0);
        if (c)
          c->parent->RemoveChild(c,R_WITHDRAW);
        break;
      case ClientMessage:
        c = ct->FindClient(ev.xclient.window,0);
        if (c && ev.xclient.message_type == wm_change_state &&
            ev.xclient.format == 32 && ev.xclient.data.l[0] == IconicState)
          c->Hide();
        break;
      case ColormapNotify:
        c = ct->FindClient(ev.xcolormap.window, 0);
        if (c && ev.xcolormap.c_new) {
          c->cmap = ev.xcolormap.colormap;
          if (ct->Focus == c)
            XInstallColormap(dpy, c->cmap);
        }
        break;
      case PropertyNotify:
        switch (ev.xproperty.atom) {
          case XA_WM_NAME:
            c = ct->FindClient(ev.xproperty.window,0);
            if (c) {
              c->ChangeName();
              c->UpdateName(false);
              if (c->icon)
                c->icon->ChangeName(0);
            }
            break;
          case XA_WM_ICON_NAME:
            c = ct->FindClient(ev.xproperty.window, 0);
            if (c && c->icon && !c->name)
              c->icon->ChangeName(0);
            break;
          case XA_WM_NORMAL_HINTS:
            c = ct->FindClient(ev.xproperty.window, 0);
            if (c) {
              long dummy;
              XGetWMNormalHints(dpy, c->window, c->size, &dummy);
#ifdef DEBUG
              c->dump();
#endif
            }
            break;
        }
        break;
      case EnterNotify:
        c = ct->FindClient(ev.xcrossing.window, 0);
        if (c) {
          if ((c->flags & CF_AUTOSHADE) && (c->flags & CF_SHADED))
            c->Shade();
          if (c->flags & CF_RAISEONENTER)
            ToBeRaised = c;
          if (c->flags & CF_FOCUSONENTER)
            WantFocus = c;
        } else {
          if (IgnoreLeave)
            break;
          for (int i=0;i!=4;++i)
            if (ev.xcrossing.window == ct->RootBorder[i]) {
              c = ct->RootDesktop;
              goto leave; // quick and dirty
            }
        }
        break;
      case LeaveNotify:
        if (IgnoreLeave)
          break;
        c = ct->FindClient(ev.xcrossing.window, 0);
        leave:
        {
          if (!c)
            break;
          if ((c->flags & CF_AUTOSHADE) && !(c->flags & CF_SHADED) && (c->frame == ev.xcrossing.window))
            c->Shade();
          Desktop *d = dynamic_cast<Desktop *>(c);
          if (!d)
            break;
          if (d->parent && (ev.xcrossing.state & ControlMask))
            d = d->parent;
          if ((ev.xcrossing.state & Mod1Mask) || (d->flags & DF_AUTOSCROLL))
            IgnoreLeave = d->Leave(ev.xcrossing.x_root,ev.xcrossing.y_root +
                       (ev.xcrossing.window==c->window ? d->THeight : 0),ev.xcrossing.state & Mod1Mask);
        }
        break;
      case Expose:
        HandleExpose(ev.xexpose);
        break;
      default:
#ifdef SHAPE
        if (shape && ev.type == shape_event) {
          c = ct->FindClient(((XShapeEvent *)&ev)->window, 0);
          if (c)
            c->SetShape();
        }
#endif
        break;
    }
#ifdef DEBUG2
    XSync(dpy,false);
#endif
    CleanUp();

  }
  Quit();
  return 0;
}

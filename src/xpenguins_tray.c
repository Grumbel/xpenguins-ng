/* SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * XEmbed system tray icon using the first frame of the Penguins bomber
 * sprite.  Click requests a graceful exit (toons explode).
 *
 * Protocol: freedesktop.org System Tray + XEmbed.  XFCE's notification
 * area still supports XEmbed when the tray is present on the panel.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/xpm.h>
#include "xpenguins.h"

#ifndef SYSTEM_TRAY_REQUEST_DOCK
#define SYSTEM_TRAY_REQUEST_DOCK 0
#endif
#ifndef XEMBED_MAPPED
#define XEMBED_MAPPED (1 << 0)
#endif

static Display *tray_dpy = None;
static Window tray_win = None;
static Window tray_owner = None;
static Pixmap tray_pixmap = None;
static Pixmap tray_mask = None;
static GC tray_gc = None;
static int tray_size = 24;
static int tray_icon_wh = 32; /* source square from bomber strip */
static Atom atom_opcode = None;
static Atom atom_xembed_info = None;
static Atom atom_manager = None;
static Atom atom_selection = None;
static char tray_docked = 0;
static char tray_enabled = 0;

static void
__tray_paint(void)
{
  if (!tray_dpy || tray_win == None)
    return;
  if (tray_pixmap != None && tray_gc != None) {
    /* Bomber strip is N frames wide x one frame tall; use first frame. */
    XCopyArea(tray_dpy, tray_pixmap, tray_win, tray_gc,
	      0, 0, (unsigned) tray_icon_wh, (unsigned) tray_icon_wh, 0, 0);
  } else {
    XSetForeground(tray_dpy, DefaultGC(tray_dpy, DefaultScreen(tray_dpy)),
		   BlackPixel(tray_dpy, DefaultScreen(tray_dpy)));
    XFillRectangle(tray_dpy, tray_win,
		   DefaultGC(tray_dpy, DefaultScreen(tray_dpy)),
		   0, 0, (unsigned) tray_size, (unsigned) tray_size);
  }
  XFlush(tray_dpy);
}

static void
__tray_set_xembed_info(void)
{
  /* CARDINAL[2]: protocol version, flags (XEMBED_MAPPED = want mapped) */
  unsigned long info[2];

  info[0] = 0;
  info[1] = XEMBED_MAPPED;
  XChangeProperty(tray_dpy, tray_win, atom_xembed_info, atom_xembed_info,
		  32, PropModeReplace, (unsigned char *) info, 2);
}

static int
__tray_send_dock(void)
{
  XEvent ev;

  if (tray_win == None || tray_owner == None)
    return -1;

  memset(&ev, 0, sizeof(ev));
  ev.xclient.type = ClientMessage;
  ev.xclient.serial = 0;
  ev.xclient.send_event = True;
  ev.xclient.display = tray_dpy;
  ev.xclient.window = tray_owner;
  ev.xclient.message_type = atom_opcode;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = CurrentTime;
  ev.xclient.data.l[1] = SYSTEM_TRAY_REQUEST_DOCK;
  ev.xclient.data.l[2] = (long) tray_win;
  ev.xclient.data.l[3] = 0;
  ev.xclient.data.l[4] = 0;
  XSendEvent(tray_dpy, tray_owner, False, NoEventMask, &ev);
  XSync(tray_dpy, False);
  tray_docked = 1;
  return 0;
}

static int
__tray_find_owner(void)
{
  tray_owner = XGetSelectionOwner(tray_dpy, atom_selection);
  return tray_owner != None;
}

static int
__tray_load_icon(void)
{
  char path[512];
  XpmAttributes xa;

  snprintf(path, sizeof(path), "%s/themes/Penguins/bomber.xpm",
	   xpenguins_directory);
  memset(&xa, 0, sizeof(xa));
  xa.valuemask = XpmSize;
  if (XpmReadFileToPixmap(tray_dpy, tray_win, path,
			  &tray_pixmap, &tray_mask, &xa) != XpmSuccess) {
    tray_pixmap = None;
    tray_mask = None;
    if (xpenguins_verbose)
      fprintf(stderr, "[xpenguins-ng] tray: could not load %s\n", path);
    return -1;
  }
  tray_gc = XCreateGC(tray_dpy, tray_win, 0, NULL);
  if (xa.height > 0)
    tray_icon_wh = (int) xa.height;
  if (tray_icon_wh > 48)
    tray_icon_wh = 32;
  tray_size = tray_icon_wh;
  return 0;
}

int
xpenguins_tray_init(Display *dpy)
{
  int screen;
  char selname[64];
  XSetWindowAttributes swa;
  XClassHint classhint;
  Atom wm_name;
  char *title = "xpenguins-ng";

  tray_dpy = dpy;
  tray_enabled = 1;
  tray_docked = 0;
  screen = DefaultScreen(dpy);

  snprintf(selname, sizeof(selname), "_NET_SYSTEM_TRAY_S%d", screen);
  atom_selection = XInternAtom(dpy, selname, False);
  atom_opcode = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
  atom_xembed_info = XInternAtom(dpy, "_XEMBED_INFO", False);
  atom_manager = XInternAtom(dpy, "MANAGER", False);

  swa.background_pixel = WhitePixel(dpy, screen);
  swa.border_pixel = BlackPixel(dpy, screen);
  swa.colormap = DefaultColormap(dpy, screen);
  swa.event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask
    | StructureNotifyMask | PropertyChangeMask;
  /* Unmapped until the tray docks us (XEMBED_MAPPED requests mapping). */
  tray_win = XCreateWindow(dpy, RootWindow(dpy, screen),
			   0, 0, (unsigned) tray_size, (unsigned) tray_size, 0,
			   DefaultDepth(dpy, screen), InputOutput,
			   DefaultVisual(dpy, screen),
			   CWBackPixel | CWBorderPixel | CWColormap | CWEventMask,
			   &swa);
  if (!tray_win)
    return -1;

  classhint.res_name = "xpenguins-ng";
  classhint.res_class = "Xpenguins-ng";
  XSetClassHint(dpy, tray_win, &classhint);
  wm_name = XInternAtom(dpy, "_NET_WM_NAME", False);
  XStoreName(dpy, tray_win, title);
  XChangeProperty(dpy, tray_win, wm_name,
		  XInternAtom(dpy, "UTF8_STRING", False), 8, PropModeReplace,
		  (unsigned char *) title, (int) strlen(title));

  __tray_set_xembed_info();
  __tray_load_icon();
  if (tray_size > 0)
    XResizeWindow(dpy, tray_win, (unsigned) tray_size, (unsigned) tray_size);

  if (__tray_find_owner()) {
    __tray_send_dock();
    if (xpenguins_verbose)
      fprintf(stderr,
	      "[xpenguins-ng] system tray docked on 0x%lx (click icon to exit)\n",
	      (unsigned long) tray_owner);
  } else {
    if (xpenguins_verbose)
      fprintf(stderr,
	      "[xpenguins-ng] no system tray yet (%s); will retry when one appears\n",
	      selname);
  }
  return 0;
}

int
xpenguins_tray_event(XEvent *event)
{
  if (!tray_enabled || !event || tray_win == None)
    return 0;

  /* Tray manager appeared or was replaced */
  if (event->type == ClientMessage
      && event->xclient.message_type == atom_manager
      && (Atom) event->xclient.data.l[1] == atom_selection) {
    tray_owner = (Window) event->xclient.data.l[2];
    if (tray_owner != None) {
      tray_docked = 0;
      __tray_set_xembed_info();
      __tray_send_dock();
      if (xpenguins_verbose)
	fprintf(stderr, "[xpenguins-ng] system tray manager ready, docked\n");
    }
    return 0;
  }

  if (event->xany.window != tray_win)
    return 0;

  switch (event->type) {
  case Expose:
    if (event->xexpose.count == 0)
      __tray_paint();
    break;
  case ConfigureNotify:
    if (event->xconfigure.width > 0) {
      tray_size = event->xconfigure.width;
      if (event->xconfigure.height > 0
	  && event->xconfigure.height < tray_size)
	tray_size = event->xconfigure.height;
    }
    __tray_paint();
    break;
  case ReparentNotify:
    /* Tray reparented us — ensure we paint once mapped */
    break;
  case MapNotify:
    __tray_paint();
    break;
  case ButtonPress:
  case ButtonRelease:
    if (event->type == ButtonRelease
	&& event->xbutton.button == Button1)
      return 1;
    break;
  default:
    break;
  }
  return 0;
}

void
xpenguins_tray_poll(void)
{
  if (!tray_enabled || tray_win == None || tray_docked)
    return;
  if (__tray_find_owner()) {
    __tray_set_xembed_info();
    __tray_send_dock();
    if (xpenguins_verbose)
      fprintf(stderr, "[xpenguins-ng] system tray found on retry, docked\n");
  }
}

void
xpenguins_tray_fini(void)
{
  if (!tray_dpy)
    return;
  if (tray_gc != None)
    XFreeGC(tray_dpy, tray_gc);
  if (tray_pixmap != None)
    XFreePixmap(tray_dpy, tray_pixmap);
  if (tray_mask != None)
    XFreePixmap(tray_dpy, tray_mask);
  if (tray_win != None)
    XDestroyWindow(tray_dpy, tray_win);
  tray_gc = None;
  tray_pixmap = tray_mask = None;
  tray_win = None;
  tray_owner = None;
  tray_dpy = NULL;
  tray_docked = 0;
  tray_enabled = 0;
}

/* SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* XEmbed system tray icon (bomber): click to request graceful exit. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/xpm.h>
#include "xpenguins.h"

static Window tray_win = None;
static Display *tray_dpy = NULL;
static Pixmap tray_pixmap = None;
static Pixmap tray_mask = None;
static GC tray_gc = None;
static int tray_size = 24;

static void
__tray_paint(void)
{
  if (!tray_dpy || tray_win == None)
    return;
  if (tray_pixmap != None && tray_gc != None) {
    XCopyArea(tray_dpy, tray_pixmap, tray_win, tray_gc,
	      0, 0, (unsigned) tray_size, (unsigned) tray_size, 0, 0);
  } else {
    XSetWindowBackground(tray_dpy, tray_win,
			 BlackPixel(tray_dpy, DefaultScreen(tray_dpy)));
    XClearWindow(tray_dpy, tray_win);
  }
}

int
xpenguins_tray_init(Display *dpy)
{
  char selname[64];
  Atom selection, opcode, data_atom;
  Window tray_owner;
  XEvent ev;
  XSetWindowAttributes swa;
  char path[512];
  XpmAttributes xa;
  int screen;

  tray_dpy = dpy;
  screen = DefaultScreen(dpy);
  snprintf(selname, sizeof(selname), "_NET_SYSTEM_TRAY_S%d", screen);
  selection = XInternAtom(dpy, selname, False);
  tray_owner = XGetSelectionOwner(dpy, selection);
  if (tray_owner == None) {
    if (xpenguins_verbose)
      fprintf(stderr, "[xpenguins-ng] no system tray (_NET_SYSTEM_TRAY_S%d)\n",
	      screen);
    return -1;
  }

  swa.background_pixel = WhitePixel(dpy, screen);
  swa.event_mask = ExposureMask | ButtonPressMask | StructureNotifyMask;
  tray_win = XCreateWindow(dpy, RootWindow(dpy, screen),
			   0, 0, (unsigned) tray_size, (unsigned) tray_size, 0,
			   CopyFromParent, InputOutput, CopyFromParent,
			   CWBackPixel | CWEventMask, &swa);
  if (!tray_win)
    return -1;

  /* Load first 32x32-ish frame from bomber; scale is approximate via clip */
  snprintf(path, sizeof(path), "%s/themes/Penguins/bomber.xpm",
	   xpenguins_directory);
  memset(&xa, 0, sizeof(xa));
  if (XpmReadFileToPixmap(dpy, tray_win, path, &tray_pixmap, &tray_mask, &xa)
      != XpmSuccess) {
    tray_pixmap = None;
    tray_mask = None;
  } else {
    tray_gc = XCreateGC(dpy, tray_win, 0, NULL);
    if (xa.width > 0 && xa.height > 0)
      tray_size = xa.height > 32 ? 32 : (int) xa.height;
  }

  opcode = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
  data_atom = XInternAtom(dpy, "_NET_SYSTEM_TRAY_MESSAGE_DATA", False);
  (void) data_atom;

  memset(&ev, 0, sizeof(ev));
  ev.xclient.type = ClientMessage;
  ev.xclient.window = tray_owner;
  ev.xclient.message_type = opcode;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = CurrentTime;
  ev.xclient.data.l[1] = 0; /* SYSTEM_TRAY_REQUEST_DOCK */
  ev.xclient.data.l[2] = (long) tray_win;
  ev.xclient.data.l[3] = 0;
  ev.xclient.data.l[4] = 0;
  XSendEvent(dpy, tray_owner, False, NoEventMask, &ev);
  XSync(dpy, False);

  if (xpenguins_verbose)
    fprintf(stderr, "[xpenguins-ng] system tray icon ready (click to exit)\n");
  return 0;
}

int
xpenguins_tray_event(XEvent *event)
{
  if (tray_win == None || !event)
    return 0;
  if (event->xany.window != tray_win)
    return 0;
  if (event->type == Expose)
    __tray_paint();
  else if (event->type == ConfigureNotify) {
    if (event->xconfigure.width > 0)
      tray_size = event->xconfigure.width;
    __tray_paint();
  } else if (event->type == ButtonPress) {
    return 1; /* request exit */
  }
  return 0;
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
  tray_dpy = NULL;
}

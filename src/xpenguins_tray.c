/* SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * XEmbed system tray icon (Penguins bomber, first frame).
 * Click = graceful exit.  Uses BackgroundPixmap so the icon stays
 * visible even when the tray never delivers Expose events (common).
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

static Display *tray_dpy;
static Window tray_win = None;
static Window tray_owner = None;
static Pixmap strip_pm = None;   /* full bomber strip */
static Pixmap strip_mask = None;
static Pixmap icon_pm = None;    /* square first frame (or drawn fallback) */
static GC tray_gc = None;
static int icon_wh = 32;
static int tray_w = 24, tray_h = 24;
static Atom atom_opcode, atom_xembed_info, atom_manager, atom_selection;
static char tray_docked;
static char tray_enabled;

static void
__tray_draw_fallback(Pixmap pm, int wh)
{
  GC gc = XCreateGC(tray_dpy, pm, 0, NULL);
  unsigned long black = BlackPixel(tray_dpy, DefaultScreen(tray_dpy));
  unsigned long white = WhitePixel(tray_dpy, DefaultScreen(tray_dpy));
  XSetForeground(tray_dpy, gc, white);
  XFillRectangle(tray_dpy, pm, gc, 0, 0, (unsigned) wh, (unsigned) wh);
  XSetForeground(tray_dpy, gc, black);
  XDrawRectangle(tray_dpy, pm, gc, 1, 1, (unsigned) (wh - 3), (unsigned) (wh - 3));
  /* Simple "X" so something is always visible */
  XDrawLine(tray_dpy, pm, gc, 4, 4, wh - 5, wh - 5);
  XDrawLine(tray_dpy, pm, gc, wh - 5, 4, 4, wh - 5);
  XFreeGC(tray_dpy, gc);
}

static void
__tray_apply_background(void)
{
  if (!tray_dpy || tray_win == None || icon_pm == None)
    return;
  XSetWindowBackgroundPixmap(tray_dpy, tray_win, icon_pm);
  XClearWindow(tray_dpy, tray_win);
  XFlush(tray_dpy);
}

static void
__tray_paint(void)
{
  if (!tray_dpy || tray_win == None || icon_pm == None)
    return;
  /* BackgroundPixmap survives most tray reparents; also copy in case not */
  XSetWindowBackgroundPixmap(tray_dpy, tray_win, icon_pm);
  XClearWindow(tray_dpy, tray_win);
  if (tray_gc != None)
    XCopyArea(tray_dpy, icon_pm, tray_win, tray_gc,
	      0, 0, (unsigned) icon_wh, (unsigned) icon_wh, 0, 0);
  XFlush(tray_dpy);
}

static void
__tray_set_xembed_info(void)
{
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
  ev.xclient.send_event = True;
  ev.xclient.display = tray_dpy;
  ev.xclient.window = tray_owner;
  ev.xclient.message_type = atom_opcode;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = CurrentTime;
  ev.xclient.data.l[1] = SYSTEM_TRAY_REQUEST_DOCK;
  ev.xclient.data.l[2] = (long) tray_win;
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
  int screen = DefaultScreen(tray_dpy);
  int depth = DefaultDepth(tray_dpy, screen);

  snprintf(path, sizeof(path), "%s/themes/Penguins/bomber.xpm",
	   xpenguins_directory);
  memset(&xa, 0, sizeof(xa));
  xa.valuemask = XpmSize | XpmVisual | XpmColormap | XpmDepth;
  xa.visual = DefaultVisual(tray_dpy, screen);
  xa.colormap = DefaultColormap(tray_dpy, screen);
  xa.depth = depth;

  if (XpmReadFileToPixmap(tray_dpy, tray_win, path,
			  &strip_pm, &strip_mask, &xa) != XpmSuccess) {
    if (xpenguins_verbose)
      fprintf(stderr, "[xpenguins-ng] tray: failed to load %s\n", path);
    strip_pm = strip_mask = None;
    icon_wh = 24;
    icon_pm = XCreatePixmap(tray_dpy, tray_win, (unsigned) icon_wh,
			    (unsigned) icon_wh, (unsigned) depth);
    __tray_draw_fallback(icon_pm, icon_wh);
  } else {
    icon_wh = (int) xa.height;
    if (icon_wh < 8)
      icon_wh = 24;
    if (icon_wh > 64)
      icon_wh = 32;
    icon_pm = XCreatePixmap(tray_dpy, tray_win, (unsigned) icon_wh,
			    (unsigned) icon_wh, (unsigned) depth);
    tray_gc = XCreateGC(tray_dpy, tray_win, 0, NULL);
    XCopyArea(tray_dpy, strip_pm, icon_pm, tray_gc,
	      0, 0, (unsigned) icon_wh, (unsigned) icon_wh, 0, 0);
    if (xpenguins_verbose)
      fprintf(stderr,
	      "[xpenguins-ng] tray: loaded bomber %dx%d, icon %dx%d from %s\n",
	      xa.width, xa.height, icon_wh, icon_wh, path);
  }
  if (tray_gc == None)
    tray_gc = XCreateGC(tray_dpy, tray_win, 0, NULL);
  tray_w = tray_h = icon_wh;
  XResizeWindow(tray_dpy, tray_win, (unsigned) tray_w, (unsigned) tray_h);
  __tray_apply_background();
  return 0;
}

int
xpenguins_tray_init(Display *dpy)
{
  int screen;
  char selname[64];
  XSetWindowAttributes swa;
  XClassHint classhint;
  const char *title = "xpenguins-ng";

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
  swa.bit_gravity = StaticGravity;
  swa.event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask
    | StructureNotifyMask | PropertyChangeMask;

  tray_win = XCreateWindow(dpy, RootWindow(dpy, screen),
			   0, 0, 24, 24, 0,
			   DefaultDepth(dpy, screen), InputOutput,
			   DefaultVisual(dpy, screen),
			   CWBackPixel | CWBorderPixel | CWColormap
			   | CWEventMask | CWBitGravity,
			   &swa);
  if (!tray_win)
    return -1;

  classhint.res_name = "xpenguins-ng";
  classhint.res_class = "Xpenguins-ng";
  XSetClassHint(dpy, tray_win, &classhint);
  XStoreName(dpy, tray_win, title);
  XChangeProperty(dpy, tray_win, XInternAtom(dpy, "_NET_WM_NAME", False),
		  XInternAtom(dpy, "UTF8_STRING", False), 8, PropModeReplace,
		  (unsigned char *) title, (int) strlen(title));

  __tray_set_xembed_info();
  __tray_load_icon();

  if (__tray_find_owner()) {
    __tray_send_dock();
    if (xpenguins_verbose)
      fprintf(stderr,
	      "[xpenguins-ng] system tray docked on 0x%lx (click to exit)\n",
	      (unsigned long) tray_owner);
  } else if (xpenguins_verbose) {
    fprintf(stderr,
	    "[xpenguins-ng] no system tray yet (%s); will retry\n", selname);
  }
  return 0;
}

int
xpenguins_tray_event(XEvent *event)
{
  if (!tray_enabled || !event || tray_win == None)
    return 0;

  if (event->type == ClientMessage
      && event->xclient.message_type == atom_manager
      && (Atom) event->xclient.data.l[1] == atom_selection) {
    tray_owner = (Window) event->xclient.data.l[2];
    if (tray_owner != None) {
      tray_docked = 0;
      __tray_set_xembed_info();
      __tray_send_dock();
      __tray_apply_background();
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
    tray_w = event->xconfigure.width;
    tray_h = event->xconfigure.height;
    __tray_apply_background();
    __tray_paint();
    break;
  case MapNotify:
  case ReparentNotify:
    __tray_apply_background();
    __tray_paint();
    break;
  case ButtonRelease:
    if (event->xbutton.button == Button1)
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
  if (!tray_enabled || tray_win == None)
    return;
  if (!tray_docked && __tray_find_owner()) {
    __tray_set_xembed_info();
    __tray_send_dock();
    if (xpenguins_verbose)
      fprintf(stderr, "[xpenguins-ng] system tray found on retry, docked\n");
  }
  /* Keep background pixmap applied; some trays clear the window. */
  if (tray_docked)
    __tray_apply_background();
}

void
xpenguins_tray_fini(void)
{
  if (!tray_dpy)
    return;
  if (tray_gc != None)
    XFreeGC(tray_dpy, tray_gc);
  if (icon_pm != None)
    XFreePixmap(tray_dpy, icon_pm);
  if (strip_pm != None)
    XFreePixmap(tray_dpy, strip_pm);
  if (strip_mask != None)
    XFreePixmap(tray_dpy, strip_mask);
  if (tray_win != None)
    XDestroyWindow(tray_dpy, tray_win);
  tray_gc = None;
  icon_pm = strip_pm = strip_mask = None;
  tray_win = tray_owner = None;
  tray_dpy = NULL;
  tray_docked = tray_enabled = 0;
}

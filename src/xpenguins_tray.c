/* SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * XEmbed system tray icon.  Uses the tray manager's preferred visual
 * (_NET_SYSTEM_TRAY_VISUAL) when present so XFCE / other panels that
 * advertise an ARGB visual can composite the icon.  Icon is drawn
 * opaque (solid panel-friendly background + first bomber frame) and
 * resized to the slot the panel assigns.  Click = graceful exit.
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
#ifndef XEMBED_EMBEDDED_NOTIFY
#define XEMBED_EMBEDDED_NOTIFY 0
#endif

static Display *tray_dpy;
static Window tray_win = None;
static Window tray_owner = None;
static Pixmap strip_pm = None;   /* full bomber strip (source frames) */
static Pixmap icon_pm = None;    /* sized opaque square ready to blit */
static GC tray_gc = None;
static Visual *tray_visual = NULL;
static Colormap tray_cmap = None;
static int tray_depth = 0;
static int icon_wh = 32;         /* preferred / current icon edge */
static int strip_h = 0;          /* height of one bomber frame */
static int tray_w = 24, tray_h = 24;
static Atom atom_opcode, atom_xembed_info, atom_xembed, atom_manager;
static Atom atom_selection, atom_tray_visual;
static char tray_docked;
static char tray_enabled;
static char tray_embedded;

/* Query tray manager for preferred visual; fall back to default. */
static void
__tray_pick_visual(int screen)
{
  Atom type = None;
  int format = 0;
  unsigned long nitems = 0, bytes_after = 0;
  unsigned char *prop = NULL;
  VisualID vid = 0;
  XVisualInfo template, *visinfo;
  int nvis = 0;

  tray_visual = DefaultVisual(tray_dpy, screen);
  tray_depth = DefaultDepth(tray_dpy, screen);
  tray_cmap = DefaultColormap(tray_dpy, screen);

  if (tray_owner == None)
    return;

  if (XGetWindowProperty(tray_dpy, tray_owner, atom_tray_visual,
			 0, 1, False, XA_VISUALID,
			 &type, &format, &nitems, &bytes_after, &prop)
      != Success || !prop || nitems < 1) {
    if (prop)
      XFree(prop);
    return;
  }

  vid = *(VisualID *) (void *) prop;
  XFree(prop);
  if (vid == 0 || vid == tray_visual->visualid)
    return;

  template.visualid = vid;
  visinfo = XGetVisualInfo(tray_dpy, VisualIDMask, &template, &nvis);
  if (!visinfo || nvis < 1) {
    if (visinfo)
      XFree(visinfo);
    return;
  }

  tray_visual = visinfo[0].visual;
  tray_depth = visinfo[0].depth;
  tray_cmap = XCreateColormap(tray_dpy, RootWindow(tray_dpy, screen),
			      tray_visual, AllocNone);
  if (xpenguins_verbose)
    fprintf(stderr,
	    "[xpenguins-ng] tray: using manager visual 0x%lx depth %d\n",
	    (unsigned long) vid, tray_depth);
  XFree(visinfo);
}

/* Build / rebuild opaque icon pixmap at the requested edge length. */
static int
__tray_build_icon(int edge)
{
  char path[512];
  XpmAttributes xa;
  int screen = DefaultScreen(tray_dpy);
  unsigned long bg, fg;
  XColor col, exact;
  int src_edge;

  if (edge < 16)
    edge = 16;
  if (edge > 64)
    edge = 64;
  icon_wh = edge;

  /* Distinct background so the slot is never “empty looking”. */
  if (XAllocNamedColor(tray_dpy, tray_cmap, "#5B9BD5", &col, &exact))
    bg = col.pixel;
  else
    bg = WhitePixel(tray_dpy, screen);
  fg = BlackPixel(tray_dpy, screen);

  if (strip_pm == None) {
    snprintf(path, sizeof(path), "%s/themes/Penguins/bomber.xpm",
	     xpenguins_directory);
    memset(&xa, 0, sizeof(xa));
    xa.valuemask = XpmSize | XpmVisual | XpmColormap | XpmDepth;
    xa.visual = tray_visual;
    xa.colormap = tray_cmap;
    xa.depth = tray_depth;

    if (XpmReadFileToPixmap(tray_dpy, tray_win, path, &strip_pm, NULL, &xa)
	!= XpmSuccess) {
      strip_pm = None;
      strip_h = 0;
      if (xpenguins_verbose)
	fprintf(stderr, "[xpenguins-ng] tray: failed to load %s\n", path);
    } else {
      strip_h = (int) xa.height;
      if (xpenguins_verbose)
	fprintf(stderr,
		"[xpenguins-ng] tray: bomber strip %dx%d\n",
		(int) xa.width, (int) xa.height);
    }
  }

  if (icon_pm != None) {
    XFreePixmap(tray_dpy, icon_pm);
    icon_pm = None;
  }
  icon_pm = XCreatePixmap(tray_dpy, tray_win, (unsigned) icon_wh,
			  (unsigned) icon_wh, (unsigned) tray_depth);
  if (tray_gc == None)
    tray_gc = XCreateGC(tray_dpy, tray_win, 0, NULL);

  XSetForeground(tray_dpy, tray_gc, bg);
  XFillRectangle(tray_dpy, icon_pm, tray_gc, 0, 0,
		 (unsigned) icon_wh, (unsigned) icon_wh);
  XSetForeground(tray_dpy, tray_gc, fg);
  XDrawRectangle(tray_dpy, icon_pm, tray_gc, 0, 0,
		 (unsigned) (icon_wh - 1), (unsigned) (icon_wh - 1));

  if (strip_pm != None && strip_h > 0) {
    /* Copy first frame, scaling by clipping when sizes differ.
     * For a proper scale we would need XRender; clip is acceptable for
     * the small tray sizes (16–32) and keeps the dependency surface
     * identical to the rest of the program. */
    src_edge = strip_h;
    if (src_edge > icon_wh)
      src_edge = icon_wh;
    XCopyArea(tray_dpy, strip_pm, icon_pm, tray_gc,
	      0, 0, (unsigned) src_edge, (unsigned) src_edge, 0, 0);
  } else {
    XDrawLine(tray_dpy, icon_pm, tray_gc, 3, 3, icon_wh - 4, icon_wh - 4);
    XDrawLine(tray_dpy, icon_pm, tray_gc, icon_wh - 4, 3, 3, icon_wh - 4);
  }

  return 0;
}

static void
__tray_paint(void)
{
  int x, y, side;

  if (!tray_dpy || tray_win == None || icon_pm == None || tray_gc == None)
    return;

  side = icon_wh;
  if (tray_w > 0 && tray_h > 0) {
    /* If the panel resized us, rebuild the icon at the new edge. */
    int want = tray_w < tray_h ? tray_w : tray_h;
    if (want != icon_wh && want >= 16)
      __tray_build_icon(want);
    side = icon_wh;
  }

  /* Fill the whole slot, then centre the square icon. */
  XSetForeground(tray_dpy, tray_gc, BlackPixel(tray_dpy, DefaultScreen(tray_dpy)));
  XFillRectangle(tray_dpy, tray_win, tray_gc, 0, 0,
		 (unsigned) (tray_w > 0 ? tray_w : side),
		 (unsigned) (tray_h > 0 ? tray_h : side));

  x = (tray_w > side) ? (tray_w - side) / 2 : 0;
  y = (tray_h > side) ? (tray_h - side) / 2 : 0;
  XCopyArea(tray_dpy, icon_pm, tray_win, tray_gc,
	    0, 0, (unsigned) side, (unsigned) side, x, y);

  /* BackgroundPixmap keeps the icon visible when Expose is sparse. */
  XSetWindowBackgroundPixmap(tray_dpy, tray_win, icon_pm);
  XFlush(tray_dpy);
}

static void
__tray_set_xembed_info(void)
{
  unsigned long info[2];
  info[0] = 0;			/* XEMBED version */
  info[1] = XEMBED_MAPPED;
  XChangeProperty(tray_dpy, tray_win, atom_xembed_info, atom_xembed_info,
		  32, PropModeReplace, (unsigned char *) info, 2);
}

static int
__tray_send_dock(void)
{
  XEvent ev;
  XSizeHints hints;

  if (tray_win == None || tray_owner == None)
    return -1;

  memset(&hints, 0, sizeof(hints));
  hints.flags = PMinSize | PBaseSize | PWinGravity;
  hints.min_width = hints.min_height = 16;
  hints.base_width = hints.base_height = icon_wh;
  hints.win_gravity = StaticGravity;
  XSetWMNormalHints(tray_dpy, tray_win, &hints);
  XResizeWindow(tray_dpy, tray_win, (unsigned) icon_wh, (unsigned) icon_wh);

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
  /* Some trays never map us unless we map ourselves after dock. */
  XMapWindow(tray_dpy, tray_win);
  tray_docked = 1;
  __tray_paint();
  return 0;
}

static int
__tray_find_owner(void)
{
  tray_owner = XGetSelectionOwner(tray_dpy, atom_selection);
  return tray_owner != None;
}

int
xpenguins_tray_init(Display *dpy)
{
  int screen;
  char selname[64];
  XSetWindowAttributes swa;
  XClassHint classhint;
  const char *title = "xpenguins-ng";
  unsigned long valuemask;

  tray_dpy = dpy;
  tray_enabled = 1;
  tray_docked = 0;
  tray_embedded = 0;
  screen = DefaultScreen(dpy);

  snprintf(selname, sizeof(selname), "_NET_SYSTEM_TRAY_S%d", screen);
  atom_selection = XInternAtom(dpy, selname, False);
  atom_opcode = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
  atom_xembed_info = XInternAtom(dpy, "_XEMBED_INFO", False);
  atom_xembed = XInternAtom(dpy, "_XEMBED", False);
  atom_manager = XInternAtom(dpy, "MANAGER", False);
  atom_tray_visual = XInternAtom(dpy, "_NET_SYSTEM_TRAY_VISUAL", False);

  if (__tray_find_owner())
    __tray_pick_visual(screen);
  else {
    tray_visual = DefaultVisual(dpy, screen);
    tray_depth = DefaultDepth(dpy, screen);
    tray_cmap = DefaultColormap(dpy, screen);
  }

  swa.background_pixel = WhitePixel(dpy, screen);
  swa.border_pixel = BlackPixel(dpy, screen);
  swa.colormap = tray_cmap;
  swa.bit_gravity = StaticGravity;
  swa.event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask
    | StructureNotifyMask | PropertyChangeMask;
  valuemask = CWBackPixel | CWBorderPixel | CWColormap
    | CWEventMask | CWBitGravity;

  tray_win = XCreateWindow(dpy, RootWindow(dpy, screen),
			   0, 0, 24, 24, 0,
			   tray_depth, InputOutput,
			   tray_visual, valuemask, &swa);
  if (!tray_win)
    return -1;

  classhint.res_name = "xpenguins-ng";
  classhint.res_class = "Xpenguins-ng";
  XSetClassHint(dpy, tray_win, &classhint);
  XStoreName(dpy, tray_win, title);

  __tray_set_xembed_info();
  __tray_build_icon(32);

  if (tray_owner != None) {
    __tray_send_dock();
    if (xpenguins_verbose)
      fprintf(stderr,
	      "[xpenguins-ng] tray: docked on 0x%lx, win 0x%lx, icon %dx%d "
	      "(opaque blue bg; click to exit)\n",
	      (unsigned long) tray_owner, (unsigned long) tray_win,
	      icon_wh, icon_wh);
  } else if (xpenguins_verbose) {
    fprintf(stderr, "[xpenguins-ng] tray: no manager for %s yet\n", selname);
  }
  return 0;
}

int
xpenguins_tray_event(XEvent *event)
{
  if (!tray_enabled || !event)
    return 0;

  if (event->type == ClientMessage
      && event->xclient.message_type == atom_manager
      && (Atom) event->xclient.data.l[1] == atom_selection) {
    tray_owner = (Window) event->xclient.data.l[2];
    if (tray_owner != None) {
      tray_docked = 0;
      /* Visual may only be known after the manager appears. */
      __tray_pick_visual(DefaultScreen(tray_dpy));
      __tray_set_xembed_info();
      __tray_send_dock();
    }
    return 0;
  }

  if (event->type == ClientMessage
      && event->xclient.message_type == atom_xembed
      && event->xclient.window == tray_win) {
    long msg = event->xclient.data.l[1];
    if (msg == XEMBED_EMBEDDED_NOTIFY) {
      tray_embedded = 1;
      if (xpenguins_verbose)
	fprintf(stderr, "[xpenguins-ng] tray: XEMBED_EMBEDDED_NOTIFY\n");
      __tray_paint();
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
    if (xpenguins_verbose)
      fprintf(stderr, "[xpenguins-ng] tray: configure %dx%d @ %d,%d\n",
	      tray_w, tray_h, event->xconfigure.x, event->xconfigure.y);
    __tray_paint();
    break;
  case ReparentNotify:
    if (xpenguins_verbose)
      fprintf(stderr, "[xpenguins-ng] tray: reparented to 0x%lx @ %d,%d\n",
	      (unsigned long) event->xreparent.parent,
	      event->xreparent.x, event->xreparent.y);
    __tray_paint();
    break;
  case MapNotify:
    if (xpenguins_verbose)
      fprintf(stderr, "[xpenguins-ng] tray: mapped\n");
    __tray_paint();
    break;
  case UnmapNotify:
    if (xpenguins_verbose)
      fprintf(stderr, "[xpenguins-ng] tray: unmapped\n");
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
    __tray_pick_visual(DefaultScreen(tray_dpy));
    __tray_set_xembed_info();
    __tray_send_dock();
  }
  if (tray_docked)
    __tray_paint();
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
  if (tray_win != None)
    XDestroyWindow(tray_dpy, tray_win);
  if (tray_cmap != None
      && tray_cmap != DefaultColormap(tray_dpy, DefaultScreen(tray_dpy)))
    XFreeColormap(tray_dpy, tray_cmap);
  tray_gc = None;
  icon_pm = strip_pm = None;
  tray_win = tray_owner = None;
  tray_visual = NULL;
  tray_cmap = None;
  tray_dpy = NULL;
  tray_docked = tray_enabled = tray_embedded = 0;
}

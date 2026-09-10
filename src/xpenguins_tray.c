/* SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/*
 * XEmbed system tray icon.  Default (opaque) screen visual so XFCE and
 * other panels can embed us without ARGB/XRender.  ParentRelative
 * background + XShape mask so the panel shows through around the
 * bomber frame.  Icon is rebuilt to the exact slot size on configure.
 * Click = graceful exit.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/xpm.h>
#include <X11/extensions/shape.h>
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
static Pixmap strip_pm = None;		/* bomber strip (colour) */
static Pixmap strip_mask = None;	/* bomber strip (bitmap mask) */
static Pixmap icon_pm = None;		/* slot-sized pixmap to blit */
static Pixmap icon_mask = None;		/* slot-sized shape mask */
static GC tray_gc = None;
static Visual *tray_visual = NULL;
static Colormap tray_cmap = None;
static int tray_depth = 0;
static int strip_w = 0, strip_h = 0;
static int tray_w = 24, tray_h = 24;
static int icon_w = 24, icon_h = 24;
static Atom atom_opcode, atom_xembed_info, atom_xembed, atom_manager;
static Atom atom_selection, atom_tray_visual;
static char tray_docked;
static char tray_enabled;
static char tray_embedded;
static char tray_have_shape;

/* Prefer the screen default visual (see previous commits for ARGB notes). */
static void
__tray_pick_visual(int screen)
{
  Atom type = None;
  int format = 0;
  unsigned long nitems = 0, bytes_after = 0;
  unsigned char *prop = NULL;
  VisualID vid = 0;

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
  if (xpenguins_verbose && vid != 0) {
    XVisualInfo template, *visinfo;
    int nvis = 0;
    template.visualid = vid;
    visinfo = XGetVisualInfo(tray_dpy, VisualIDMask, &template, &nvis);
    if (visinfo && nvis > 0) {
      fprintf(stderr,
	      "[xpenguins-ng] tray: manager offers visual 0x%lx depth %d "
	      "(using default depth %d; ParentRelative + XShape)\n",
	      (unsigned long) vid, visinfo[0].depth, tray_depth);
      XFree(visinfo);
    } else {
      if (visinfo)
	XFree(visinfo);
    }
  }
}

/* Load bomber strip once (colour + 1-bit mask). */
static void
__tray_load_strip(void)
{
  char path[512];
  XpmAttributes xa;

  if (strip_pm != None)
    return;

  snprintf(path, sizeof(path), "%s/themes/Penguins/bomber.xpm",
	   xpenguins_directory);
  memset(&xa, 0, sizeof(xa));
  xa.valuemask = XpmSize | XpmVisual | XpmColormap | XpmDepth;
  xa.visual = tray_visual;
  xa.colormap = tray_cmap;
  xa.depth = tray_depth;

  if (XpmReadFileToPixmap(tray_dpy, tray_win, path, &strip_pm, &strip_mask,
			  &xa) != XpmSuccess) {
    strip_pm = None;
    strip_mask = None;
    strip_w = strip_h = 0;
    if (xpenguins_verbose)
      fprintf(stderr, "[xpenguins-ng] tray: failed to load %s\n", path);
    return;
  }
  strip_w = (int) xa.width;
  strip_h = (int) xa.height;
  if (xpenguins_verbose)
    fprintf(stderr, "[xpenguins-ng] tray: bomber strip %dx%d%s\n",
	    strip_w, strip_h, strip_mask != None ? " (with mask)" : "");
}

/* Build slot-sized colour pixmap + matching 1-bit shape mask. */
static int
__tray_build_icon(int width, int height)
{
  int frame, dx, dy, copy_w, copy_h;
  GC mask_gc;

  if (width < 1)
    width = 16;
  if (height < 1)
    height = 16;
  icon_w = width;
  icon_h = height;

  __tray_load_strip();

  if (icon_pm != None) {
    XFreePixmap(tray_dpy, icon_pm);
    icon_pm = None;
  }
  if (icon_mask != None) {
    XFreePixmap(tray_dpy, icon_mask);
    icon_mask = None;
  }

  icon_pm = XCreatePixmap(tray_dpy, tray_win, (unsigned) icon_w,
			  (unsigned) icon_h, (unsigned) tray_depth);
  if (tray_gc == None)
    tray_gc = XCreateGC(tray_dpy, tray_win, 0, NULL);

  /* Fill under the sprite; shape mask hides outside the bomber frame. */
  XSetForeground(tray_dpy, tray_gc,
		 BlackPixel(tray_dpy, DefaultScreen(tray_dpy)));
  XFillRectangle(tray_dpy, icon_pm, tray_gc, 0, 0,
		 (unsigned) icon_w, (unsigned) icon_h);

  frame = (strip_h > 0) ? strip_h : 24;
  copy_w = frame < icon_w ? frame : icon_w;
  copy_h = frame < icon_h ? frame : icon_h;
  dx = (icon_w - copy_w) / 2;
  dy = (icon_h - copy_h) / 2;

  if (strip_pm != None && strip_h > 0) {
    XCopyArea(tray_dpy, strip_pm, icon_pm, tray_gc,
	      0, 0, (unsigned) copy_w, (unsigned) copy_h, dx, dy);
  } else {
    XSetForeground(tray_dpy, tray_gc,
		   WhitePixel(tray_dpy, DefaultScreen(tray_dpy)));
    XDrawLine(tray_dpy, icon_pm, tray_gc, 3, 3, icon_w - 4, icon_h - 4);
    XDrawLine(tray_dpy, icon_pm, tray_gc, icon_w - 4, 3, 3, icon_h - 4);
  }

  /* 1-bit mask: empty, then copy first-frame mask centred. */
  icon_mask = XCreatePixmap(tray_dpy, tray_win, (unsigned) icon_w,
			    (unsigned) icon_h, 1);
  mask_gc = XCreateGC(tray_dpy, icon_mask, 0, NULL);
  XSetForeground(tray_dpy, mask_gc, 0);
  XFillRectangle(tray_dpy, icon_mask, mask_gc, 0, 0,
		 (unsigned) icon_w, (unsigned) icon_h);
  if (strip_mask != None && strip_h > 0) {
    XCopyArea(tray_dpy, strip_mask, icon_mask, mask_gc,
	      0, 0, (unsigned) copy_w, (unsigned) copy_h, dx, dy);
  } else {
    XSetForeground(tray_dpy, mask_gc, 1);
    XFillRectangle(tray_dpy, icon_mask, mask_gc, 0, 0,
		   (unsigned) icon_w, (unsigned) icon_h);
  }
  XFreeGC(tray_dpy, mask_gc);
  return 0;
}

static void
__tray_apply_shape(void)
{
  if (!tray_have_shape || tray_win == None || icon_mask == None)
    return;
  XShapeCombineMask(tray_dpy, tray_win, ShapeBounding, 0, 0, icon_mask,
		    ShapeSet);
}

static void
__tray_paint(void)
{
  int x, y, side_w, side_h;

  if (!tray_dpy || tray_win == None)
    return;

  if (tray_w > 0 && tray_h > 0
      && (tray_w != icon_w || tray_h != icon_h))
    __tray_build_icon(tray_w, tray_h);
  else if (icon_pm == None)
    __tray_build_icon(tray_w > 0 ? tray_w : 24, tray_h > 0 ? tray_h : 24);

  if (icon_pm == None || tray_gc == None)
    return;

  side_w = icon_w;
  side_h = icon_h;
  x = (tray_w > side_w) ? (tray_w - side_w) / 2 : 0;
  y = (tray_h > side_h) ? (tray_h - side_h) / 2 : 0;

  /* ParentRelative: panel colour shows through where the shape is empty. */
  XSetWindowBackgroundPixmap(tray_dpy, tray_win, ParentRelative);
  XClearWindow(tray_dpy, tray_win);

  XCopyArea(tray_dpy, icon_pm, tray_win, tray_gc,
	    0, 0, (unsigned) side_w, (unsigned) side_h, x, y);
  __tray_apply_shape();
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
  XSizeHints hints;

  if (tray_win == None || tray_owner == None)
    return -1;

  memset(&hints, 0, sizeof(hints));
  hints.flags = PMinSize | PBaseSize | PWinGravity;
  hints.min_width = hints.min_height = 16;
  hints.base_width = hints.base_height = 24;
  hints.win_gravity = StaticGravity;
  XSetWMNormalHints(tray_dpy, tray_win, &hints);

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
  /* Leave unmapped until XEMBED_EMBEDDED_NOTIFY. */
  tray_docked = 1;
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
  int screen, shape_event, shape_error;
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
  tray_have_shape = XShapeQueryExtension(dpy, &shape_event, &shape_error)
    ? 1 : 0;

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

  swa.background_pixmap = ParentRelative;
  swa.border_pixel = BlackPixel(dpy, screen);
  swa.colormap = tray_cmap;
  swa.bit_gravity = StaticGravity;
  swa.event_mask = ExposureMask | ButtonPressMask | ButtonReleaseMask
    | StructureNotifyMask | PropertyChangeMask;
  valuemask = CWBackPixmap | CWBorderPixel | CWColormap
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
  __tray_build_icon(24, 24);

  if (tray_owner != None) {
    __tray_send_dock();
    if (xpenguins_verbose)
      fprintf(stderr,
	      "[xpenguins-ng] tray: docked on 0x%lx, win 0x%lx, "
	      "icon %dx%d (ParentRelative + shape; click to exit)\n",
	      (unsigned long) tray_owner, (unsigned long) tray_win,
	      icon_w, icon_h);
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
      XMapWindow(tray_dpy, tray_win);
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
  if (tray_docked && tray_embedded)
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
  if (icon_mask != None)
    XFreePixmap(tray_dpy, icon_mask);
  if (strip_pm != None)
    XFreePixmap(tray_dpy, strip_pm);
  if (strip_mask != None)
    XFreePixmap(tray_dpy, strip_mask);
  if (tray_win != None)
    XDestroyWindow(tray_dpy, tray_win);
  tray_gc = None;
  icon_pm = icon_mask = strip_pm = strip_mask = None;
  tray_win = tray_owner = None;
  tray_visual = NULL;
  tray_cmap = None;
  tray_dpy = NULL;
  tray_docked = tray_enabled = tray_embedded = 0;
}

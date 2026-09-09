/* toon_init.c - initialising various things
 * Copyright (C) 1999-2001  Robin Hogan
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#ifdef HAVE_XFIXES
#include <X11/extensions/Xfixes.h>
#endif
#include "toon.h"


/* STARTUP FUNCTIONS */

/* Return 1 if a compositing manager owns _NET_WM_CM_Sn */
static int
__ToonCompositorRunning(Display *dpy, int screen)
{
  char sel[32];
  Atom atom;
  snprintf(sel, sizeof(sel), "_NET_WM_CM_S%d", screen);
  atom = XInternAtom(dpy, sel, False);
  return XGetSelectionOwner(dpy, atom) != None;
}

/* Find a 32-bit TrueColor visual suitable for a transparent overlay */
static int
__ToonFindArgbVisual(Display *dpy, int screen, XVisualInfo *vi_out)
{
  XVisualInfo template;
  XVisualInfo *list;
  int n, i;

  template.screen = screen;
  template.depth = 32;
  template.class = TrueColor;
  list = XGetVisualInfo(dpy, VisualScreenMask | VisualDepthMask | VisualClassMask,
                        &template, &n);
  if (!list)
    return 0;

  for (i = 0; i < n; i++) {
    if (list[i].red_mask && list[i].green_mask && list[i].blue_mask) {
      *vi_out = list[i];
      XFree(list);
      return 1;
    }
  }
  XFree(list);
  return 0;
}

/* Make the window ignore pointer events (click-through), unless squish is on */
static void
__ToonSetClickThrough(Display *dpy, Window win)
{
  if (toon_squish)
    return;

#ifdef HAVE_XFIXES
  {
    int major = 0, minor = 0, event_base, error_base;
    if (XFixesQueryExtension(dpy, &event_base, &error_base)) {
      if (XFixesQueryVersion(dpy, &major, &minor) && major >= 2) {
        XserverRegion region = XFixesCreateRegion(dpy, NULL, 0);
        XFixesSetWindowShapeRegion(dpy, win, ShapeInput, 0, 0, region);
        XFixesDestroyRegion(dpy, region);
        return;
      }
    }
  }
#endif
  /* Fallback: empty ShapeInput via XShape */
  {
    int event_base, error_base;
    if (XShapeQueryExtension(dpy, &event_base, &error_base)) {
      XShapeCombineRectangles(dpy, win, ShapeInput, 0, 0, NULL, 0,
                              ShapeSet, Unsorted);
    }
  }
}

/*
 * Choose the drawable used for toon pixels.
 * Classic path: draw on toon_root (desktop / virtual root).
 * Overlay path: full-screen override-redirect ARGB window (compositor-friendly).
 * Returns 0 on success, -1 if overlay was requested but could not be created
 * (falls back to classic).
 */
int
ToonSetupDrawWindow(void)
{
  int screen;
  int want_overlay;
  XVisualInfo vi;
  XSetWindowAttributes swa;
  Colormap cmap;
  Window overlay;
  Atom opacity_atom;
  unsigned long opacity;

  toon_draw_window = toon_root;
  toon_overlay_mode = 0;

  screen = DefaultScreen(toon_display);

  /* --id forces classic drawing on the given window */
  if (toon_root_override) {
    snprintf(toon_message, TOON_MESSAGE_LENGTH,
             _("Drawing on user-specified window (classic path)"));
    return 0;
  }

  if (toon_overlay_preference > 0)
    want_overlay = 1;
  else if (toon_overlay_preference == 0)
    want_overlay = 0;
  else
    want_overlay = __ToonCompositorRunning(toon_display, screen);

  if (!want_overlay) {
    snprintf(toon_message, TOON_MESSAGE_LENGTH,
             _("Drawing on desktop window (classic path)"));
    return 0;
  }

  if (!__ToonFindArgbVisual(toon_display, screen, &vi)) {
    snprintf(toon_message, TOON_MESSAGE_LENGTH,
             _("No ARGB visual; falling back to classic root drawing"));
    return -1;
  }

  cmap = XCreateColormap(toon_display, RootWindow(toon_display, screen),
                       vi.visual, AllocNone);

  swa.colormap = cmap;
  swa.background_pixel = 0; /* fully transparent */
  swa.border_pixel = 0;
  swa.override_redirect = True;
  swa.event_mask = toon_squish ? ButtonPressMask : 0;

  overlay = XCreateWindow(toon_display,
                          RootWindow(toon_display, screen),
                          0, 0,
                          (unsigned) toon_display_width,
                          (unsigned) toon_display_height,
                          0,
                          vi.depth,
                          InputOutput,
                          vi.visual,
                          CWColormap | CWBackPixel | CWBorderPixel |
                          CWOverrideRedirect | CWEventMask,
                          &swa);
  if (!overlay) {
    snprintf(toon_message, TOON_MESSAGE_LENGTH,
             _("Failed to create overlay window; using classic path"));
    return -1;
  }

  /* Hint compositors this is a desktop/background surface */
  {
    Atom type = XInternAtom(toon_display, "_NET_WM_WINDOW_TYPE", False);
    Atom desktop = XInternAtom(toon_display, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
    XChangeProperty(toon_display, overlay, type, XA_ATOM, 32,
                    PropModeReplace, (unsigned char *) &desktop, 1);
  }

  opacity_atom = XInternAtom(toon_display, "_NET_WM_WINDOW_OPACITY", False);
  opacity = 0xffffffffUL;
  XChangeProperty(toon_display, overlay, opacity_atom, XA_CARDINAL, 32,
                  PropModeReplace, (unsigned char *) &opacity, 1);

  __ToonSetClickThrough(toon_display, overlay);

  XLowerWindow(toon_display, overlay);
  XMapWindow(toon_display, overlay);
  XFlush(toon_display);

  toon_draw_window = overlay;
  toon_overlay_mode = 1;
  snprintf(toon_message, TOON_MESSAGE_LENGTH,
           _("Using transparent overlay window (compositor mode)"));
  return 0;
}



/* Resize overlay (and refresh display metrics) if the desktop size changed */
void
ToonSyncDisplaySize(void)
{
  XWindowAttributes attributes;
  int w, h;

  XGetWindowAttributes(toon_display, toon_root, &attributes);
  w = attributes.width;
  h = attributes.height;

  if (toon_root != toon_parent) {
    toon_x_offset = attributes.x;
    toon_y_offset = attributes.y;
  }

  if (w == toon_display_width && h == toon_display_height)
    return;

  toon_display_width = w;
  toon_display_height = h;

  if (toon_overlay_mode && toon_draw_window) {
    XMoveResizeWindow(toon_display, toon_draw_window, 0, 0,
                      (unsigned) w, (unsigned) h);
    XLowerWindow(toon_display, toon_draw_window);
  }

  if (toon_squish_window) {
    XMoveResizeWindow(toon_display, toon_squish_window, 0, 0,
                      (unsigned) w, (unsigned) h);
    XLowerWindow(toon_display, toon_squish_window);
  }
}

/* Open display */
Display *
ToonOpenDisplay(char *display_name)
{
  toon_display=XOpenDisplay(display_name);
  if (toon_display == NULL) {
    if (display_name == NULL && getenv("DISPLAY") == NULL)
      strncpy(toon_error_message, _("DISPLAY environment variable not set"),
	      TOON_MESSAGE_LENGTH);
    else
      strncpy(toon_error_message, _("Can't open display"),
	      TOON_MESSAGE_LENGTH);
    return(NULL);
  }
  ToonInit(toon_display);
  return toon_display;
}

/* Setup graphics context and create some XRegions */
/* Currently this function always returns 0 */
int
ToonInit(Display *d)
{
  int screen = 0;
  XGCValues gc_values;
  XWindowAttributes attributes;

  toon_display = d;

  screen = DefaultScreen(toon_display);

  *toon_message = '\0';
  if (toon_root_override) {
    toon_root = toon_parent = toon_root_override;
  }
  else {
    toon_root = ToonGetRootWindow(toon_display, screen, &toon_parent);
  }
  XGetWindowAttributes(toon_display, toon_root, &attributes);
  toon_display_width = attributes.width;
  toon_display_height = attributes.height;
  if (toon_root != toon_parent) {
    /* Work out the position of toon_root with respect to toon_parent;
     * assume for now that toon_parent is the same size as the root
     * window */
    toon_x_offset = attributes.x;
    toon_y_offset = attributes.y;
  }

  /* Choose classic root drawing or a transparent overlay window */
  ToonSetupDrawWindow();

  /* If we want to squish the toons with the mouse then we must create
   * a window over the root window that has the same properties.
   * In overlay mode with squish, ButtonPress is selected on the overlay
   * itself, so no extra InputOnly window is needed. */
  if (toon_squish && !toon_overlay_mode) {
    XSetWindowAttributes squish_att;
    squish_att.event_mask = ButtonPressMask;
    squish_att.override_redirect = True;
    toon_squish_window = XCreateWindow(toon_display, toon_root, 0, 0,
				       toon_display_width, toon_display_height,
				       0, CopyFromParent, InputOnly, CopyFromParent,
				       CWOverrideRedirect | CWEventMask,
				       &squish_att);
    XDefineCursor(toon_display, toon_squish_window,
		  XCreateFontCursor(toon_display, XC_target));
    XLowerWindow(toon_display, toon_squish_window);
  }

  /* Is anyone interested in this window? If so we must inform them of
   * where the toons are by sending expose events - that way they can
   * redraw themselves when a toon walks over them */
  if (attributes.all_event_masks & ExposureMask) {
    int len = strlen(toon_message);
    toon_expose = 1;
    if (*toon_message) {
      snprintf(toon_message + len, TOON_MESSAGE_LENGTH - len,
	       _(" and redrawing overwritten desktop icons"));
    }
    else {
      snprintf(toon_message, TOON_MESSAGE_LENGTH,
	       _("Redrawing overwritten desktop icons"));
    }
    toon_message[TOON_MESSAGE_LENGTH-1] = '\0';
  }
  else {
    toon_expose = 0;
  }
  
  /* Set Graphics Context */
  gc_values.function = GXcopy;
  gc_values.graphics_exposures = False;
  gc_values.fill_style = FillTiled;
  toon_drawGC = XCreateGC(toon_display, toon_draw_window,
			  GCFunction | GCFillStyle | GCGraphicsExposures,
			  &gc_values);

  /* Regions */
  toon_windows = XCreateRegion();

  /* Notify if the location of the client windows changes,
     or if the window we are drawing to changes size.
     Always listen for StructureNotify on the X root so we can resize
     the overlay when the screen geometry changes (e.g. RandR). */
  {
    Window xroot = RootWindow(toon_display, screen);
    if (toon_root != xroot) {
      if (toon_root == toon_parent) {
        XSelectInput(toon_display, toon_root, SubstructureNotifyMask
                     | StructureNotifyMask);
      }
      else {
        XSelectInput(toon_display, toon_root, StructureNotifyMask);
        XSelectInput(toon_display, toon_parent, SubstructureNotifyMask);
      }
      XSelectInput(toon_display, xroot, StructureNotifyMask);
    }
    else {
      XSelectInput(toon_display, toon_parent,
                   SubstructureNotifyMask | StructureNotifyMask);
    }
  }

  toon_nwindows = 0;
  
  if (toon_squish_window) {
    XMapWindow(toon_display, toon_squish_window);
  }

  return 0;
}

/* Configure signal handling and the way the toons behave via a bitmask */
/* Currently always returns 0 */
int
ToonConfigure(unsigned long int code)
{
  if (code & TOON_EDGEBLOCK)
    toon_edge_block=1;
  else if (code & TOON_SIDEBOTTOMBLOCK)
    toon_edge_block=2;
  else if (code & TOON_NOEDGEBLOCK)
    toon_edge_block=0;

  if (code & TOON_SOLIDPOPUPS)
    toon_solid_popups=1;
  else if (code & TOON_NOSOLIDPOPUPS)
    toon_solid_popups=0;

  if (code & TOON_SHAPEDWINDOWS)
    toon_shaped_windows=1;
  else if (code & TOON_NOSHAPEDWINDOWS)
    toon_shaped_windows=0;

  if (code & TOON_CATCHSIGNALS) {
    signal(SIGINT, __ToonSignalHandler);
    signal(SIGTERM, __ToonSignalHandler);
    signal(SIGHUP, __ToonSignalHandler);
  }
  else if (code & TOON_EXITGRACEFULLY) {
    signal(SIGINT, __ToonExitGracefully);
    signal(SIGTERM, __ToonExitGracefully);
    signal(SIGHUP, __ToonExitGracefully);
  }
  else if (code & TOON_NOCATCHSIGNALS) {
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGHUP, SIG_DFL);
  }

  if (code & TOON_SQUISH) {
    toon_squish = 1;
  }
  else if (code & TOON_NOSQUISH) {
    toon_squish = 0;
  }

  if (code & TOON_OVERLAY) {
    toon_overlay_preference = 1;
  }
  else if (code & TOON_NOOVERLAY) {
    toon_overlay_preference = 0;
  }

  return 0;
}

/* Store the pixmaps to the server */
/* Returns 0 on success, otherwise the return value from the Xpm function */
int
ToonInstallData(ToonData **data, int ngenera, int ntypes)
{
  int i, j, status;
  XpmAttributes attributes;
  attributes.valuemask = (XpmReturnPixels
			  | XpmReturnExtensions | XpmExactColors 
			  | XpmCloseness);
  attributes.exactColors=False;
  attributes.closeness=40000;
  for (i = 0; i < ngenera; ++i) {
    for (j = 0; j < ntypes; ++j) {
      ToonData *d = data[i]+j;
      if (d->exists && !d->master) {
	if ((status =
	     XpmCreatePixmapFromData(toon_display, toon_draw_window,
				     d->image,
				     &(d->pixmap), 
				     &(d->mask), 
				     &attributes))) {
	  return status;
	}
      }
    }
    /* Loop through the types again for any pixmaps that are copies */
    for (j = 0; j < ntypes; ++j) {
      ToonData *d = data[i]+j;
      if (d->exists && d->master) {
	d->pixmap = d->master->pixmap;
	d->mask = d->master->mask;
      }
    }
  }

  toon_data = data;
  toon_ngenera = ngenera;
  toon_ntypes = ntypes;
  return 0;
}

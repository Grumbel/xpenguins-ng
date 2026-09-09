/* toon_core.c - core functions for advancing a frame of the animation
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
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xatom.h>
#include "toon.h"

/* Error handler for X */
int
__ToonXErrorHandler(Display *display, XErrorEvent *error)
{
  toon_errno = error->error_code;
  return 0;
}

/* CORE FUNCTIONS */

/* Attempt to move a toon based on its velocity */
/* `mode' can be TOON_MOVE (move unless blocked), TOON_FORCE (move
   regardless) or TOON_STILL (test the move but don't actually do it) */
/* Returns TOON_BLOCKED if blocked, TOON_OK if unblocked, or 
   TOON_PARTIALMOVE if limited movement is possible */
int
ToonAdvance(Toon *toon, int mode)
{
  int newx, newy;
  int new_zone;
  unsigned int width, height;
  int move_ahead = 1;
  int result = TOON_OK;
  ToonData *data = toon_data[toon->genus] + toon->type;
  char nocycle = (( (data->conf) & TOON_NOCYCLE ) > 0);
  char stationary = 0;

  if (mode == TOON_STILL) move_ahead = 0;

  width = data->width;
  height = data->height;

  newx = toon->x + toon->u;
  newy = toon->y + toon->v;

  stationary = (toon->u == 0 && toon->v == 0);

  if (data->conf & TOON_NOBLOCK) {
    /* Just consider blocking by the sides of the screen */
    if (toon_edge_block) {
      if (newx < 0) {
	newx = 0;
	result=TOON_PARTIALMOVE;
      }
      else if (newx + data->width > toon_display_width) {
	newx=toon_display_width-data->width;
	result=TOON_PARTIALMOVE;
      }
    }
  }
  else {
    /* Consider all blocking */
    if (toon_edge_block) {
      if (newx < 0) {
	newx = 0;
	result=TOON_PARTIALMOVE;
      }
      else if (newx + data->width > toon_display_width) {
	newx=toon_display_width-data->width;
	result=TOON_PARTIALMOVE;
      }
      if (newy < 0 && toon_edge_block != 2) {
	newy=0;
	result=TOON_PARTIALMOVE;
      }
      else if (newy + data->height > toon_display_height) {
	newy=toon_display_height-data->height;
	result=TOON_PARTIALMOVE;
      }
      if (newx == toon->x && newy == toon->y && !stationary) {
	result=TOON_BLOCKED;
      }
    }

    /* Is new toon location fully/partially filled with windows? */
    new_zone = XRectInRegion(toon_windows,newx,newy,width,height);
    if (new_zone != RectangleOut && mode == TOON_MOVE 
	&& result != TOON_BLOCKED && !stationary) {
      int tryx, tryy, step=1, u=newx-toon->x, v=newy-toon->y;
      result=TOON_BLOCKED;
      move_ahead=0;
      /* How far can we move the toon? */
      if ( abs(v) < abs(u) ) {
	if (newx>toon->x) step=-1;
	for (tryx = newx+step; tryx != (toon->x); tryx += step) {
	  tryy=toon->y+((tryx-toon->x)*(v))/(u);
	  if (XRectInRegion(toon_windows,tryx,tryy,width,height) == RectangleOut) {
	    newx=tryx;
	    newy=tryy;
	    result=TOON_PARTIALMOVE;
	    move_ahead=1;
	    break;
	  }
	}
      }
      else {
	if (newy>toon->y) step=-1;
	for (tryy=newy+step;tryy!=(toon->y);tryy=tryy+step) {
	  tryx=toon->x+((tryy-toon->y)*(u))/(v);
	  if (XRectInRegion(toon_windows,tryx,tryy,width,height) == RectangleOut) {
	    newx=tryx;
	    newy=tryy;
	    result=TOON_PARTIALMOVE;
	    move_ahead=1;
	    break;
	  }
	}
      }
    }
  }

  if (move_ahead) {
    toon->x=newx;
    toon->y=newy;
    if ( (++(toon->frame)) >= data->nframes) {
      toon->frame = 0;
      ++(toon->cycle);
      if (nocycle) { 
	toon->active = 0;
      }
    }
  }
  else if (nocycle) {
    if ( (++(toon->frame)) >= data->nframes) {
      toon->frame = 0;
      toon->cycle = 0;
      toon->active = 0;
    }
  }
  return result;
}

/* Build up an X-region corresponding to the location of the windows 
   that we don't want our toons to enter */
/* Returns 0 on success, 1 if windows moved again during the execution
   of this function */
/* True if window is a desktop/wallpaper surface (not walkable terrain). */
static int
__ToonIsDesktopWindow(Display *dpy, Window w)
{
  Atom type_atom, desktop_atom, actual_type;
  int actual_format;
  unsigned long nitems, bytesafter;
  Atom *atoms = NULL;
  int i, is_desktop = 0;

  type_atom = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", True);
  desktop_atom = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DESKTOP", True);
  if (type_atom == None || desktop_atom == None)
    return 0;

  if (XGetWindowProperty(dpy, w, type_atom, 0, 16, False, XA_ATOM,
			 &actual_type, &actual_format, &nitems, &bytesafter,
			 (unsigned char **) &atoms) == Success && atoms) {
    for (i = 0; i < (int) nitems; i++) {
      if (atoms[i] == desktop_atom) {
	is_desktop = 1;
	break;
      }
    }
    XFree(atoms);
  }
  return is_desktop;
}

int
ToonLocateWindows(void)
{
  Window *children = NULL;
  Window dummy;
  XWindowAttributes attributes;
  int wx;
  XRectangle *window_rect;
  int x, y;
  unsigned int height, width;
  unsigned int oldnwindows;

  XRectangle *rects = NULL;
  int nrects, rectord, irect;
  XSetErrorHandler(__ToonXErrorHandler);

  /* Rebuild window region */
  XDestroyRegion(toon_windows);
  toon_windows = XCreateRegion();

  /* Get children of root */
  oldnwindows = toon_nwindows;
  wx = XQueryTree(toon_display, toon_parent, &dummy, &dummy, &children, &toon_nwindows);

  if (toon_nwindows > oldnwindows) {
    if (toon_windata)
      free(toon_windata);
    if ((toon_windata = calloc(toon_nwindows, sizeof(__ToonWindowData)))
	== NULL) {
      fprintf(stderr, _("Error: out of memory\n"));
      __ToonExitGracefully(1);
    }
  }

  /* Refresh display size (and resize overlay if needed) */
  ToonSyncDisplaySize();

  /*
   * First pass: record geometry of candidate solid windows.
   * XQueryTree lists children bottom-to-top; we need that order later
   * to subtract occlusion so only *visible* window surface is solid.
   */
  for (wx = 0; wx < (int) toon_nwindows; wx++) {
    toon_errno = 0;

    toon_windata[wx].wid = children[wx];
    toon_windata[wx].solid = 0;

    XGetWindowAttributes(toon_display, children[wx], &attributes);
    if (toon_errno) continue;

    /* Popup? */
    if ((!toon_solid_popups) && attributes.save_under) continue;

    /* Never treat our own overlay (or squish) window as solid terrain */
    if (toon_overlay_mode && children[wx] == toon_draw_window)
      continue;
    if (toon_squish_window && children[wx] == toon_squish_window)
      continue;

    /* Desktop/wallpaper is not terrain */
    if (__ToonIsDesktopWindow(toon_display, children[wx])) {
      if (toon_debug)
	fprintf(stderr, "[xpenguins] skip DESKTOP window 0x%lx\n",
		(unsigned long) children[wx]);
      continue;
    }
    if (children[wx] == toon_root && toon_root != toon_parent) {
      if (toon_debug)
	fprintf(stderr, "[xpenguins] skip root/background 0x%lx\n",
		(unsigned long) children[wx]);
      continue;
    }

    if (attributes.map_state == IsViewable) {
      x = attributes.x - toon_x_offset;
      y = attributes.y - toon_y_offset;
      width = attributes.width + 2 * attributes.border_width;
      height = attributes.height + 2 * attributes.border_width;

      /* Entirely offscreen? */
      if (x >= toon_display_width) continue;
      if (y >= toon_display_height) continue;
      if (y + (int) height <= 0) continue;
      if (x + (int) width <= 0) continue;

      /*
       * Windows with y <= 0 (maximized, wallpapers, top strips) have a
       * top edge at the screen edge. Walkers there sit at y = -height and
       * are fully off-screen. Classic xpenguins skipped these so penguins
       * fall through maximized windows and walk on y > 0 tops / panels.
       */
      if (y <= 0) {
	if (toon_debug)
	  fprintf(stderr,
		  "[xpenguins] skip y<=0 window 0x%lx (%d,%d) %ux%u\n",
		  (unsigned long) children[wx], x, y, width, height);
	continue;
      }

      /* Low-stack near-fullscreen: wallpaper without DESKTOP type */
      if (wx < 5
	  && width >= (unsigned) toon_display_width / 3
	  && height >= (unsigned) toon_display_height - 32) {
	if (toon_debug)
	  fprintf(stderr,
		  "[xpenguins] skip full-screen low-stack 0x%lx (%d,%d) %ux%u\n",
		  (unsigned long) children[wx], x, y, width, height);
	continue;
      }

      toon_windata[wx].solid = 1;
      window_rect = &(toon_windata[wx].pos);
      window_rect->x = x;
      window_rect->y = y;
      window_rect->height = height;
      window_rect->width = width;
      if (toon_debug)
	fprintf(stderr,
		"[xpenguins] solid candidate 0x%lx (%d,%d) %ux%u stack=%d\n",
		(unsigned long) children[wx], x, y, width, height, wx);
    }
  }

  /*
   * Second pass: top-most window first. Visible solid area is the
   * window region minus everything already covered by higher windows.
   */
  {
    Region covered = XCreateRegion();
    Region win_reg = XCreateRegion();
    Region visible = XCreateRegion();

    for (wx = (int) toon_nwindows - 1; wx >= 0; wx--) {
      if (!toon_windata[wx].solid)
	continue;

      XDestroyRegion(win_reg);
      win_reg = XCreateRegion();
      window_rect = &(toon_windata[wx].pos);

      if (!toon_shaped_windows) {
	XUnionRectWithRegion(window_rect, win_reg, win_reg);
      } else {
	rects = XShapeGetRectangles(toon_display, toon_windata[wx].wid,
				    ShapeBounding, &nrects, &rectord);
	if (nrects <= 1) {
	  XUnionRectWithRegion(window_rect, win_reg, win_reg);
	} else {
	  for (irect = 0; irect < nrects; irect++) {
	    rects[irect].x += window_rect->x;
	    rects[irect].y += window_rect->y;
	    XUnionRectWithRegion(rects + irect, win_reg, win_reg);
	  }
	}
	if (rects && nrects > 0)
	  XFree(rects);
      }

      XSubtractRegion(win_reg, covered, visible);
      XUnionRegion(toon_windows, visible, toon_windows);
      XUnionRegion(covered, win_reg, covered);
    }

    XDestroyRegion(covered);
    XDestroyRegion(win_reg);
    XDestroyRegion(visible);
  }

  if (toon_debug) {
    XRectangle box;
    int nsolid = 0;
    for (wx = 0; wx < (int) toon_nwindows; wx++)
      if (toon_windata[wx].solid)
	nsolid++;
    XClipBox(toon_windows, &box);
    fprintf(stderr,
	    "[xpenguins] locate: display %dx%d, children %u, solid %d, "
	    "region box (%d,%d) %dx%d, empty=%d\n",
	    toon_display_width, toon_display_height,
	    (unsigned) toon_nwindows, nsolid,
	    box.x, box.y, box.width, box.height,
	    XEmptyRegion(toon_windows));
  }

  XFree(children);
  XSetErrorHandler((__ToonErrorHandler *) NULL);
  return 0;
}

/* Wait for a specified number of microseconds */
int
ToonSleep(unsigned long usecs)
{
  struct timeval t;
  t.tv_usec = usecs%(unsigned long)1000000;
  t.tv_sec = usecs/(unsigned long)1000000;
  select(0, (void *)0, (void *)0, (void *)0, &t);
  return 0;
}

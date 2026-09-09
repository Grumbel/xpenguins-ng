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
int
ToonLocateWindows()
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
  oldnwindows=toon_nwindows;
  wx = XQueryTree(toon_display, toon_parent, &dummy, &dummy, &children, &toon_nwindows);

  if (toon_nwindows>oldnwindows) {
    if (toon_windata)
      free(toon_windata);
    if ((toon_windata = calloc(toon_nwindows, sizeof(__ToonWindowData)))
	== NULL) {
      fprintf(stderr, _("Error: out of memory\n"));
      __ToonExitGracefully(1);
    }
  }

  /* Check to see if toon_root has moved with respect to toon_parent */
  XGetWindowAttributes(toon_display, toon_root, &attributes);
  toon_display_width = attributes.width;
  toon_display_height = attributes.height;
  if (toon_root != toon_parent) {
    toon_x_offset = attributes.x;
    toon_y_offset = attributes.y;
  }

  /* Add windows to region */
  for (wx=0; wx<toon_nwindows; wx++) {
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

    if (attributes.map_state == IsViewable) {
      /* Geometry of the window, borders inclusive */

      x = attributes.x - toon_x_offset;
      y = attributes.y - toon_y_offset;
      width = attributes.width + 2*attributes.border_width;
      height = attributes.height + 2*attributes.border_width;

      /* Entirely offscreen? */
      if (x >= toon_display_width) continue;
      if (y >= toon_display_height) continue;
      if (y <= 0) continue;
      if ((x + width) < 0) continue;

      toon_windata[wx].solid = 1;
      window_rect = &(toon_windata[wx].pos);
      window_rect->x = x;
      window_rect->y = y;
      window_rect->height = height;
      window_rect->width = width;
      /* The area of the windows themselves */
      if (!toon_shaped_windows) {
	XUnionRectWithRegion(window_rect, toon_windows, toon_windows);
      }
      else {
	rects = XShapeGetRectangles(toon_display, children[wx], ShapeBounding,
				    &nrects, &rectord);
	if (nrects <= 1) {
	  XUnionRectWithRegion(window_rect, toon_windows, toon_windows);
	}
	else {
	  for (irect=0;irect<nrects;irect++) {
	    rects[irect].x += x;
	    rects[irect].y += y;
	    XUnionRectWithRegion(rects+irect, toon_windows, toon_windows);
	  }
	}
	if ((rects) && (nrects > 0)) {
	  XFree(rects);
	}
      }
    }
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

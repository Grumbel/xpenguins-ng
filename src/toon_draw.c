/* toon_draw.c - draw and erase the toons 
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
#include "toon.h"
/* DRAWING FUNCTIONS */

/* Draw the toons from toon[0] to toon[n-1] */
/* Currently always returns 0 */
int
ToonDraw(Toon *t, int n)
{
  int i;
  for (i = 0; i < n; i++, t++) {
    if (t->active) {
      ToonData *data = toon_data[t->genus] + t->type;
      int width = data->width;
      int height = data->height;
      int direction = t->direction;
      if (direction >= data->ndirections) {
	direction = 0;
      }

      XSetClipOrigin(toon_display, toon_drawGC,
		     t->x-width*t->frame, t->y-height*direction); 
      XSetClipMask(toon_display, toon_drawGC, data->mask);   
      XCopyArea(toon_display, data->pixmap,
		toon_root,toon_drawGC,width*t->frame,height*direction,
		width,height,t->x,t->y);
      XSetClipMask(toon_display, toon_drawGC, None);
      t->x_map = t->x;
      t->y_map = t->y;
      t->width_map = width;
      t->height_map = height;
      t->mapped = 1;
    }
    else {
      t->mapped = 0;
    }
  }
  return 0;
}

/* Erase toons toon[0] to toon[n-1] */
/* Currently always returns 0 */
/* If toon_expose is set then every 100th frame an expose event will
 * be sent to redraw any desktop icons */
int
ToonErase(Toon *t, int n)
{
  static int minx = 10000;
  static int maxx = 0;
  static int miny = 10000;
  static int maxy = 0;
  static int count = 0;

  int i;

  for (i = 0; i < n; i++, t++) {
    if (t->mapped) {
      int x = t->x_map;
      int y = t->y_map;
      int width = t->width_map;
      int height = t->height_map;
      XClearArea(toon_display, toon_root, x, y,
		 width, height, False);
      if (toon_expose) {
	if (x < minx) {
	  minx = x;
	}
	if (x + width > maxx) {
	  maxx = x + width;
	}
	if (y < miny) {
	  miny = y;
	}
	if (y + height > maxy) {
	  maxy = y + height;
	}
      }
    }
  }

  if (toon_expose && count > 100
      && maxx > minx && maxy > miny) {
    XExposeEvent event;

    event.type        = Expose;
    event.send_event  = True;
    event.display     = toon_display;
    event.window      = toon_root;
    event.x           = minx;
    event.y           = miny;
    event.width       = maxx-minx + 1;
    event.height      = maxy-miny + 1;
    XSendEvent(toon_display, toon_root, True, Expose,
	       (XEvent *) &event);
    minx = 10000;
    maxx = 0;
    miny = 10000;
    maxy = 0;
    count = 0;
  }
  else {
    ++count;
  }

  return 0;
}

/* Send any buffered X calls immediately */
void
ToonFlush()
{
  XFlush(toon_display);
  return;
}

/* SPDX-FileCopyrightText: 1999-2001 Robin Hogan
 * SPDX-FileCopyrightText: 2026 xpenguins-ng contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

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
#include <X11/extensions/shape.h>
#include "toon.h"

/* Rebuild ShapeBounding so only toon pixels are visible (overlay mode). */
static void
__ToonUpdateOverlayShape(Toon *t, int n)
{
  int i;
  int event_base, error_base;

  if (!toon_overlay_mode || !toon_draw_window)
    return;
  if (!XShapeQueryExtension(toon_display, &event_base, &error_base))
    return;

  /* Start from an empty bounding region (fully transparent / not shown) */
  XShapeCombineRectangles(toon_display, toon_draw_window, ShapeBounding,
                          0, 0, NULL, 0, ShapeSet, Unsorted);

  for (i = 0; i < n; i++) {
    Toon *toon = t + i;
    ToonData *data;
    int direction;
    unsigned int width, height;
    int src_x, src_y;
    Pixmap frame_mask;
    GC mask_gc;

    if (!toon->mapped)
      continue;

    data = toon_data[toon->genus] + toon->type;
    width = data->width;
    height = data->height;
    direction = toon->direction;
    if (direction >= (int) data->ndirections)
      direction = 0;
    src_x = (int) width * toon->frame;
    src_y = (int) height * direction;

    if (data->mask == None) {
      XRectangle rect;
      rect.x = toon->x_map;
      rect.y = toon->y_map;
      rect.width = toon->width_map;
      rect.height = toon->height_map;
      XShapeCombineRectangles(toon_display, toon_draw_window, ShapeBounding,
                              0, 0, &rect, 1, ShapeUnion, Unsorted);
      continue;
    }

    frame_mask = XCreatePixmap(toon_display, toon_draw_window, width, height, 1);
    mask_gc = XCreateGC(toon_display, frame_mask, 0, NULL);
    XSetForeground(toon_display, mask_gc, 0);
    XFillRectangle(toon_display, frame_mask, mask_gc, 0, 0, width, height);
    XCopyPlane(toon_display, data->mask, frame_mask, mask_gc,
               src_x, src_y, width, height, 0, 0, 1);
    /*
     * Xpm clip masks: set bits = opaque (draw here).  ShapeBounding:
     * set bits = part of the window.  Under real compositors the 1-bit
     * mask from Xpm came out inverted for shaping (black where the
     * sprite is transparent, holes on the body).  Invert for shape.
     */
    XSetFunction(toon_display, mask_gc, GXinvert);
    XFillRectangle(toon_display, frame_mask, mask_gc, 0, 0, width, height);
    XSetFunction(toon_display, mask_gc, GXcopy);

    XShapeCombineMask(toon_display, toon_draw_window, ShapeBounding,
                      toon->x_map, toon->y_map, frame_mask, ShapeUnion);
    XFreeGC(toon_display, mask_gc);
    XFreePixmap(toon_display, frame_mask);
  }
}

/* Draw the toons from toon[0] to toon[n-1] */
/* Currently always returns 0 */
int
ToonDraw(Toon *t, int n)
{
  int i;
  Toon *base = t;

  /*
   * Overlay: assign map geometry first, then set ShapeBounding, then
   * paint.  Changing the shape *after* XCopyArea causes newly included
   * regions to be filled with the window background (black), wiping the
   * coloured pixels and leaving solid black silhouettes.
   */
  if (toon_overlay_mode) {
    for (i = 0; i < n; i++) {
      if (base[i].active) {
	ToonData *data = toon_data[base[i].genus] + base[i].type;
	base[i].x_map = base[i].x;
	base[i].y_map = base[i].y;
	base[i].width_map = data->width;
	base[i].height_map = data->height;
	base[i].mapped = 1;
      } else {
	base[i].mapped = 0;
      }
    }
    __ToonUpdateOverlayShape(base, n);
  }

  for (i = 0; i < n; i++, t++) {
    if (t->active) {
      ToonData *data = toon_data[t->genus] + t->type;
      int width = data->width;
      int height = data->height;
      int direction = t->direction;
      if (direction >= (int) data->ndirections) {
	direction = 0;
      }

      /*
       * Classic path: clip to the Xpm mask so we only paint opaque
       * pixels onto the root/desktop.
       * Overlay path: paint the full colour frame from data->pixmap
       * (not the mask).  ShapeBounding already limits what is visible.
       */
      if (!toon_overlay_mode) {
	XSetClipOrigin(toon_display, toon_drawGC,
		       t->x - width * t->frame, t->y - height * direction);
	XSetClipMask(toon_display, toon_drawGC, data->mask);
      }
      XCopyArea(toon_display, data->pixmap,
		toon_draw_window, toon_drawGC,
		width * t->frame, height * direction,
		width, height, t->x, t->y);
      if (!toon_overlay_mode)
	XSetClipMask(toon_display, toon_drawGC, None);
      if (!toon_overlay_mode) {
	t->x_map = t->x;
	t->y_map = t->y;
	t->width_map = width;
	t->height_map = height;
	t->mapped = 1;
      }
    }
    else if (!toon_overlay_mode) {
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
      /* Overlay: old pixels drop out of ShapeBounding on the next
       * ToonDraw; clearing is unnecessary and with background None
       * would not help anyway. */
      if (!toon_overlay_mode)
	XClearArea(toon_display, toon_draw_window, x, y,
		   width, height, False);
      if (toon_expose && !toon_overlay_mode) {
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

  if (toon_expose && !toon_overlay_mode) {
    count++;
    if (count >= 100 && maxx > minx && maxy > miny) {
      XExposeEvent event;
      event.type         = Expose;
      event.serial       = 0;
      event.send_event   = True;
      event.display      = toon_display;
      event.window       = toon_root;
      event.x            = minx;
      event.y            = miny;
      event.width        = maxx - minx;
      event.height       = maxy - miny;
      event.count        = 0;
      XSendEvent(toon_display, toon_root, True, ExposureMask,
		 (XEvent *) &event);
      count = 0;
      minx = 10000;
      maxx = 0;
      miny = 10000;
      maxy = 0;
    }
  }
  return 0;
}

void
ToonFlush()
{
  XFlush(toon_display);
}

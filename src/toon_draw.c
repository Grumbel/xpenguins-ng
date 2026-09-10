/* SPDX-FileCopyrightText: 1999-2001 Robin Hogan
 * SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
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
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include "toon.h"

/* Wallpaper pixmap published by modern desktop managers (feh, xfdesktop,
 * nitrogen, ...).  XClearArea only restores a window *background*, which
 * those tools often leave unset — so classic mode left trails. */
static Pixmap toon_root_pmap = None;
static int toon_root_pmap_w = 0, toon_root_pmap_h = 0;

static void
__ToonRefreshRootPixmap(void)
{
  Atom atoms[2];
  Atom type = None;
  int format = 0;
  unsigned long nitems = 0, bytes_after = 0;
  unsigned char *prop = NULL;
  int a;

  atoms[0] = XInternAtom(toon_display, "_XROOTPMAP_ID", True);
  atoms[1] = XInternAtom(toon_display, "ESETROOT_PMAP_ID", True);

  toon_root_pmap = None;
  toon_root_pmap_w = toon_root_pmap_h = 0;

  for (a = 0; a < 2; a++) {
    if (atoms[a] == None)
      continue;
    {
      Window xroot = RootWindow(toon_display, DefaultScreen(toon_display));
      prop = NULL;
      nitems = 0;
      /* Property lives on the X root; we may draw on a virtual desktop. */
      if (XGetWindowProperty(toon_display, xroot, atoms[a], 0, 1, False,
			     XA_PIXMAP, &type, &format, &nitems, &bytes_after,
			     &prop) != Success) {
	prop = NULL;
	nitems = 0;
      }
    }
    if (prop && nitems >= 1) {
      toon_root_pmap = *((Pixmap *) (void *) prop);
      XFree(prop);
      prop = NULL;
      if (toon_root_pmap != None) {
	Window root_ret;
	int x, y;
	unsigned int w, h, bw, depth;
	if (XGetGeometry(toon_display, toon_root_pmap, &root_ret,
			 &x, &y, &w, &h, &bw, &depth)) {
	  toon_root_pmap_w = (int) w;
	  toon_root_pmap_h = (int) h;
	  return;
	}
	toon_root_pmap = None;
      }
    } else if (prop) {
      XFree(prop);
      prop = NULL;
    }
  }
}

/* Restore wallpaper under a rectangle; fall back to XClearArea. */
static void
__ToonClearRect(int x, int y, unsigned int width, unsigned int height)
{
  if (width == 0 || height == 0)
    return;

  {
    static int refresh_cd = 0;
    if (toon_root_pmap == None || --refresh_cd <= 0) {
      __ToonRefreshRootPixmap();
      refresh_cd = 300; /* re-check wallpaper every few seconds */
    }
  }

  if (toon_root_pmap != None && toon_root_pmap_w > 0 && toon_root_pmap_h > 0) {
    int sx = x, sy = y;
    int dx = x, dy = y;
    int w = (int) width, h = (int) height;

    /* Root pixmap is often sized to the virtual desktop; clamp. */
    if (sx < 0) {
      w += sx;
      dx -= sx;
      sx = 0;
    }
    if (sy < 0) {
      h += sy;
      dy -= sy;
      sy = 0;
    }
    if (sx + w > toon_root_pmap_w)
      w = toon_root_pmap_w - sx;
    if (sy + h > toon_root_pmap_h)
      h = toon_root_pmap_h - sy;
    if (w > 0 && h > 0) {
      XCopyArea(toon_display, toon_root_pmap, toon_draw_window, toon_drawGC,
		sx, sy, (unsigned) w, (unsigned) h, dx, dy);
      return;
    }
  }

  XClearArea(toon_display, toon_draw_window, x, y, width, height, False);
}

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
       * Always clip to the Xpm mask so only opaque sprite pixels are
       * written.  In overlay mode ShapeBounding makes the window
       * outline match the toons, but all toons share one pixmap: a
       * full-rectangle XCopyArea would still stamp "transparent"
       * (typically black) XPM pixels over any other toon underneath.
       */
      if (data->mask != None) {
	XSetClipOrigin(toon_display, toon_drawGC,
		       t->x - width * t->frame, t->y - height * direction);
	XSetClipMask(toon_display, toon_drawGC, data->mask);
      }
      XCopyArea(toon_display, data->pixmap,
		toon_draw_window, toon_drawGC,
		width * t->frame, height * direction,
		width, height, t->x, t->y);
      if (data->mask != None)
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
	__ToonClearRect(x, y, (unsigned) width, (unsigned) height);
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

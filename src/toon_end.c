/* SPDX-FileCopyrightText: 1999-2001 Robin Hogan
 * SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* toon_end.c - functions for finishing up
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
#include <stdlib.h>
#include "toon.h"

/* FINISHING UP */

/* Free client side and server side data */
int
ToonFinishUp()
{
  XExposeEvent event;

  if (toon_windows) {
    XDestroyRegion(toon_windows);
    toon_windows = NULL;
  }
  if (toon_draw_window)
    XClearWindow(toon_display, toon_draw_window);
  /* When drawing on the desktop root, send expose so icons redraw */
  if (!toon_overlay_mode) {
    event.type = Expose;
    event.send_event = True;
    event.display = toon_display;
    event.window = toon_root;
    event.x = 0;
    event.y = 0;
    event.width = toon_display_width;
    event.height = toon_display_height;
    XSendEvent(toon_display, toon_root, False, Expose, (XEvent *) &event);
  }

  ToonFreeData();
  if (toon_drawGC) {
    XFreeGC(toon_display,toon_drawGC);
    toon_drawGC = NULL;
  }
  if (toon_windata) {
    free(toon_windata);
    toon_windata=NULL;
  }

  if (toon_squish_window) {
    XDestroyWindow(toon_display, toon_squish_window);
    toon_squish_window = (Window) 0;
  }

  if (toon_overlay_mode && toon_draw_window) {
    XDestroyWindow(toon_display, toon_draw_window);
    toon_draw_window = 0;
    toon_overlay_mode = 0;
  }

  return 0;
}

/* Close link to X server */
int
ToonCloseDisplay()
{
  ToonFinishUp();
  XCloseDisplay(toon_display);
  return 0;
}

/* Delete the pixmaps on the server */
/* Returns 0 on success, 1 if there is no data to free*/
int
ToonFreeData()
{
  int i, j;
  if (toon_data == NULL || toon_ntypes == 0 || toon_ngenera == 0) {
    return 1;
  }
  for (i = 0; i < toon_ngenera; ++i) {
    for (j = 0; j < toon_ntypes; ++j) {
      ToonData *data = toon_data[i] + j;
      if (data->exists && !data->master) {
	XFreePixmap(toon_display, data->pixmap);
	XFreePixmap(toon_display, data->mask);
      }
    }
  }
  toon_data = NULL;
  toon_ntypes = 0;
  toon_ngenera = 0;
  return 0;
}

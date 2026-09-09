/* SPDX-FileCopyrightText: 1999-2001 Robin Hogan
 * SPDX-FileCopyrightText: 2026 xpenguins-ng contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* toon_query.c - query functions
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

/* QUERY FUNCTIONS */

/* Returns 1 if the toon is blocked in the specified direction, 0 if not 
   blocked and -1 if the direction arguments was out of bounds */
int
ToonBlocked(Toon *toon, int direction)
{
  ToonData *data = toon_data[toon->genus] + toon->type;
  if (toon_edge_block) {
    switch (direction) {
    case TOON_LEFT:
      if ((toon->x) <= 0)
	return 1;
      break;
    case TOON_RIGHT:
      if ((toon->x)+(data->width) >= toon_display_width)
	return 1;
      break;
    case TOON_UP:
      if ((toon->y) <= 0)
	return 1;
      break;
    case TOON_DOWN:
      if ((toon->y)+(data->height) >= toon_display_height)
	return 1;
      break;
    }
  }
  switch (direction) {
  case TOON_HERE:
    return (XRectInRegion(toon_windows,toon->x,toon->y,
			  data->width,
			  data->height) 
	    != RectangleOut);
  case TOON_LEFT:
    return (XRectInRegion(toon_windows,toon->x-1,toon->y,
			  1,data->height) 
	    != RectangleOut);
  case TOON_RIGHT:
    return (XRectInRegion(toon_windows,toon->x+data->width,
			  toon->y,1,data->height) 
	    != RectangleOut);
  case TOON_UP:
    return (XRectInRegion(toon_windows,toon->x,toon->y-1,
			  data->width,1)
	    != RectangleOut);
  case TOON_DOWN:
    return (XRectInRegion(toon_windows,toon->x,
			  toon->y+data->height,
			  data->width, 1)
	    != RectangleOut);
  default:
    return -1;
  }
}

/* Returns 1 if the toon would be in an occupied area if moved by xoffset
   and yoffset, 0 otherwise */
int
ToonOffsetBlocked(Toon *toon, int xoffset, int yoffset)
{
  ToonData *data = toon_data[toon->genus] + toon->type;
  if (toon_edge_block) {
    if (  ((toon->x + xoffset) <= 0)
	  || ((toon->x)+(data->width + xoffset) 
	      >= toon_display_width) 
	  || ((toon->y + yoffset) <= 0 && toon_edge_block != 2) 
	  || ((toon->y)+(data->height + yoffset) 
	      >= toon_display_height) ) {
      return 1;
    }
  }
  return (XRectInRegion(toon_windows,toon->x + xoffset,toon->y + yoffset,
			data->width, data->height) 
	  != RectangleOut);
}

/* Check to see if a toon would be squashed instantly if changed to
   certain type, return 1 if squashed, 0 otherwise. Useful to call
   before ToonSetType().  */
int
ToonCheckBlocked(Toon *toon, int type, int gravity)
{
  ToonData *data = toon_data[toon->genus] + toon->type;
  ToonData *newdata = toon_data[toon->genus] + type;
  int x = toon->x;
  int y = toon->y;
  switch(gravity) {
  case TOON_HERE:
    x += (int) (data->width - newdata->width)/2;
    y += (int) (data->height - newdata->height)/2;
    break;
  case TOON_DOWN:
    x += (int) (data->width - newdata->width)/2;
    y += (int) (data->height - newdata->height);
    break;
  case TOON_UP:
    x += (int) (data->width - newdata->width)/2;
    break;
  case TOON_LEFT:
    y += (int) (data->height - newdata->height)/2;
    break;
  case TOON_RIGHT:
    x += (int) (data->width - newdata->width);
    y += (int) (data->height - newdata->height)/2;
    break;
  case TOON_DOWNLEFT:
    y += (int) (data->height - newdata->height);
    break;
  case TOON_DOWNRIGHT:
    x += (int) (data->width - newdata->width);
    y += (int) (data->height - newdata->height);
    break;
  case TOON_UPRIGHT:
    x += (int) (data->width - newdata->width);
    break;
  } /* Otherwise already in the right position */

  return (XRectInRegion(toon_windows, x, y, newdata->width, newdata->height)
	  != RectangleOut);
}

/* Returns 1 if any change to the top-level window configuration has occurred,
   0 otherwise */
int
ToonWindowsMoved(void)
{
  XEvent event;
  int windows_moved = 0;

  while (XPending(toon_display)) {
    XNextEvent(toon_display, &event);
    switch (event.type) {
    case ConfigureNotify:
    case MapNotify:
    case UnmapNotify:
    case CreateNotify:
    case DestroyNotify:
    case ReparentNotify:
    case CirculateNotify:
      windows_moved = 1;
      break;
    case ButtonPress:
      toon_button_x = ((XButtonEvent *) &event)->x;
      toon_button_y = ((XButtonEvent *) &event)->y;
      break;
    default:
      break;
    }
  }
  return windows_moved;
}

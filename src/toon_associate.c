/* SPDX-FileCopyrightText: 1999-2001 Robin Hogan
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* toon_associate.c - handle toon associations with moving windows
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

/* HANDLING TOON ASSOCIATIONS WITH MOVING WINDOWS */

/* Set a toons association direction - e.g. TOON_DOWN if the toon
   is walking along the tops the window, TOON_UNASSOCIATED if
   the toon is in free space */
void
ToonSetAssociation(Toon *toon, int direction)
{
  toon->associate = direction;
  return;
}

void
ToonSetMaximumRelocate(int up, int down, int left, int right) {
  toon_max_relocate_up = up;
  toon_max_relocate_down = down;
  toon_max_relocate_left = left;
  toon_max_relocate_right = right;
  return;
}

/* The first thing to be done when the windows move is to work out 
   which windows the associated toons were associated with just before
   the windows moved */
/* Currently this function always returns 0 */
int
ToonCalculateAssociations(Toon *toon, int n)
{
  int i, wx;
  int x, y;
  int width, height;

  for (i=0; i<n; i++) {
    if (toon[i].associate != TOON_UNASSOCIATED && toon[i].active) {
      ToonData *data = toon_data[toon[i].genus] + toon[i].type;
      /* determine the position of a line of pixels that the associated
	 window should at least partially enclose */
      switch (toon[i].associate) {
      case TOON_DOWN:
	x = toon[i].x;
	y = toon[i].y + data->height;
	width = data->width;
	height = 1;
	break;
      case TOON_UP:
	x = toon[i].x;
	y = toon[i].y - 1;
	width = data->width;
	height = 1;
	break;
      case TOON_LEFT:
	x = toon[i].x - 1;
	y = toon[i].y;
	width = 1;
	height = data->height;
	break;
      case TOON_RIGHT:
	x = toon[i].x + data->width;
	y = toon[i].y;
	width = 1;
	height = data->height;
	break;
      default:
	fprintf(stderr, _("Error: illegal direction: %d\n"),toon[i].associate);
	__ToonExitGracefully(1);
	return 1; /* avoid `gcc -Wall' thinking x etc might be used uninitialised */
      }

      toon[i].wid = 0;
      for (wx=0; wx<toon_nwindows; wx++) {
	if (toon_windata[wx].solid &&
	    toon_windata[wx].pos.x < x+width && 
	    toon_windata[wx].pos.x + toon_windata[wx].pos.width > x &&
	    toon_windata[wx].pos.y < y+height &&
	    toon_windata[wx].pos.y + toon_windata[wx].pos.height > y) {
	  toon[i].wid = toon_windata[wx].wid;
	  toon[i].xoffset = toon[i].x - toon_windata[wx].pos.x;
	  toon[i].yoffset = toon[i].y - toon_windata[wx].pos.y;
	  break;
	}
      }
    }
  }
  return 0;
}

/* After calling ToonLocateWindows() we relocate all the toons that were
   associated with particular windows */
/* Currently this function always returns 0 */
int
ToonRelocateAssociated(Toon *toon, int n)
{
  int i, wx, dx, dy;
  for (i=0; i<n; i++) {
    if (toon[i].associate != TOON_UNASSOCIATED && toon[i].wid != 0
	&& toon[i].active) {
      for (wx=0; wx<toon_nwindows; wx++) {
	if (toon[i].wid == toon_windata[wx].wid && toon_windata[wx].solid) {
	  dx = toon[i].xoffset + toon_windata[wx].pos.x - toon[i].x;
	  dy = toon[i].yoffset + toon_windata[wx].pos.y - toon[i].y;
	  if (dx < toon_max_relocate_right && -dx < toon_max_relocate_left
	      && dy < toon_max_relocate_down && -dy < toon_max_relocate_up) {
	    if (!ToonOffsetBlocked(toon+i, dx, dy)) {
	      toon[i].x += dx;
	      toon[i].y += dy;
	    }
	  }
	  break;
	}
      }
    } 
  }
  return 0;
}

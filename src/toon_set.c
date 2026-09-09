/* toon_set.c - assignment functions
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
#include "toon.h"

/* ASSIGNMENT FUNCTIONS */

/* Move a toon */
void
ToonMove(Toon *toon, int xoffset, int yoffset)
{
  toon->x += xoffset;
  toon->y += yoffset;
  return;
}

/* Directly assign the position of a toon */
void
ToonSetPosition(Toon *toon, int x, int y)
{
  toon->x=x;
  toon->y=y;
  return;
}

/* Change a toons genus and type and activate it. */
/* Gravity determines position offset of toon if size different from
 * previous type.
 * Note that there is a ToonSetType macro which doesn't change the genus. */
void
ToonSetGenusAndType(Toon *toon, int genus, int type, int direction, int gravity)
{
  ToonData *data = toon_data[toon->genus] + toon->type;
  ToonData *newdata = toon_data[genus] + type;
  switch(gravity) {
  case TOON_HERE:
    toon->x += (int) (data->width - newdata->width)/2;
    toon->y += (int) (data->height - newdata->height)/2;
    break;
  case TOON_DOWN:
    toon->x += (int) (data->width - newdata->width)/2;
    toon->y += (int) (data->height - newdata->height);
    break;
  case TOON_UP:
    toon->x += (int) (data->width - newdata->width)/2;
    break;
  case TOON_LEFT:
    toon->y += (int) (data->height - newdata->height)/2;
    break;
  case TOON_RIGHT:
    toon->x += (int) (data->width - newdata->width);
    toon->y += (int) (data->height - newdata->height)/2;
    break;
  case TOON_DOWNLEFT:
    toon->y += (int) (data->height - newdata->height);
    break;
  case TOON_DOWNRIGHT:
    toon->x += (int) (data->width - newdata->width);
    toon->y += (int) (data->height - newdata->height);
    break;
  case TOON_UPRIGHT:
    toon->x += (int) (data->width - newdata->width);
    break;
  } /* Otherwise already in the right position */
  toon->type = type;
  toon->genus = genus;
  toon->cycle = 0;
  toon->direction = direction;
  toon->frame = 0;
  toon->active = 1;
  return;

}

/* Set toon velocity */
void
ToonSetVelocity(Toon *toon, int u, int v)
{
  toon->u=u;
  toon->v=v;
  return;
}

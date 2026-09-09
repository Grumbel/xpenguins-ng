/* xpenguins_core.c - provides the core functionality of xpenguins
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
#include <time.h>
#include <stdlib.h>
#include "xpenguins.h"

/* Random integer between 0 and maxint-1 */
#define RandInt(maxint) ((int) ((maxint)*((float) rand()/(RAND_MAX+1.0))))

/* Global variables */
Toon penguin[PENGUIN_MAX];
ToonData **penguin_data = NULL;
unsigned int *penguin_numbers = NULL;
int penguin_number = 0;
unsigned int penguin_ngenera = 0;
char xpenguins_active = 0;
char xpenguins_blood = 1; /* 0 = suitable for children */
char xpenguins_angels = 1; /* 0 = no angels */
char xpenguins_specify_number = 0;

/* Start a new penguin from the top of the screen */
static
void
__xpenguins_init_penguin(Toon *p)
{
  ToonData *data = penguin_data[p->genus] + PENGUIN_FALLER;
  p->direction = RandInt(2);
  ToonSetType(p, PENGUIN_FALLER, p->direction,
	      TOON_UNASSOCIATED);
  /* Fully above the screen (y + height == 0).  The old value
   * (1 - height) still overlapped any solid window at y==0 by one
   * pixel; the faller then became a walker while embedded and the
   * next frame's TOON_HERE check exploded it. */
  ToonSetPosition(p, RandInt(ToonDisplayWidth()
			     - data->width),
		  -((int) data->height));
  if (toon_debug)
    fprintf(stderr, "[xpenguins-ng] spawn faller at (%d,%d) size %ux%u\n",
            p->x, p->y, data->width, data->height);
  ToonSetAssociation(p, TOON_UNASSOCIATED);
  ToonSetVelocity(p, (p->direction)*2-1, data->speed);
  p->terminating = 0;
}


/* If a toon is embedded in solid window area, nudge it upward until free.
 * Used after a faller/tumbler "lands" so the following walker frame is
 * not immediately TOON_HERE-squashed. */
static void
__xpenguins_lift_out_of_solid(Toon *p)
{
  int guard = 0;
  while (ToonBlocked(p, TOON_HERE) && guard++ < toon_display_height) {
    p->y -= 1;
  }
}

/* Turn a penguin into a climber */
static
void
__xpenguins_make_climber(Toon *p)
{
  if (p->direction) {
    ToonSetType(p, PENGUIN_CLIMBER, p->direction,
		TOON_DOWNRIGHT);
  }
  else {
    ToonSetType(p, PENGUIN_CLIMBER, p->direction,
		TOON_DOWNLEFT);
  }
  ToonSetAssociation(p, p->direction);
  ToonSetVelocity(p, 0, -penguin_data[p->genus][PENGUIN_CLIMBER].speed);
}

/* Turn a penguin into a walker. To ensure that a climber turning into
 * a walker does not lose its footing, set shiftforward to 1
 * (otherwise 0) */
static
void
__xpenguins_make_walker(Toon *p, int shiftforward)
{
  unsigned int newtype = PENGUIN_WALKER;
  int gravity = TOON_DOWN;

  if (shiftforward) {
    if (p->direction) {
      gravity = TOON_DOWNRIGHT;
    }
    else {
      gravity = TOON_DOWNLEFT;
    }
  }

  if (penguin_data[p->genus][PENGUIN_RUNNER].exists && !RandInt(4)) {
    newtype = PENGUIN_RUNNER;
    /* Sometimes runners are larger than walkers - check for immediate
       squash */
    if (ToonCheckBlocked(p, newtype, gravity)) {
      newtype = PENGUIN_WALKER;
    }
  }

  ToonSetType(p, newtype, p->direction, gravity);
  ToonSetAssociation(p, TOON_DOWN);
  ToonSetVelocity(p, penguin_data[p->genus][newtype].speed
		  * ((2 * p->direction) - 1), 0);
}

/* Turn a penguin into a faller */
static
void
__xpenguins_make_faller(Toon *p)
{
  ToonSetVelocity(p, (p->direction)*2 - 1,
		  penguin_data[p->genus][PENGUIN_FALLER].speed);
  ToonSetType(p, PENGUIN_FALLER, p->direction, TOON_UP);
  ToonSetAssociation(p, TOON_UNASSOCIATED);
}

/* Connect to X server and upload data */
char *
xpenguins_start(char *display_name)
{
  if (!penguin_data) {
    return _("No toon data installed");
  }
  if (!xpenguins_active) {
    int i, index, imod = 1;
    unsigned long configure_mask = TOON_SIDEBOTTOMBLOCK;

    /* reset random-number generator */
    srand(time((long *) NULL));
    if (!ToonOpenDisplay(display_name)) {
      return toon_error_message;
    }
    if (xpenguins_verbose && *toon_message) {
      fprintf(stderr, "%s\n", toon_message);
    }

    /* Set up various preferences: Edge of screen is solid,
     * and if a signal is caught then exit the main event loop */
    ToonConfigure(configure_mask);

    /* Set the distance the window can move (up, down, left, right)
     * and penguin can still cling on */
    ToonSetMaximumRelocate(16,16,16,16);

    /* Send the pixmaps to the X server - penguin_data should have been 
     * defined in penguins/def.h */
    ToonInstallData(penguin_data, penguin_ngenera, PENGUIN_NTYPES);

    if (!xpenguins_specify_number) {
      penguin_number = 0;
      for (i = 0; i < penguin_ngenera; ++i) {
	penguin_number += penguin_numbers[i];
      }
    }
    /* Set the genus of each penguin, whether it is to be activated or not */
    for (index = 0; index < penguin_ngenera && index < PENGUIN_MAX; ++index) {
      penguin[index].genus = index;
    }
    while (index < PENGUIN_MAX) {
      for (i = 0; i < penguin_ngenera; ++i) {
	int j;
	for (j = 0; j < penguin_numbers[i]-imod && index < PENGUIN_MAX; ++j) {
	  penguin[index++].genus = i;
	}
      }
      imod = 0;
    }
    /* Initialise penguins */
    for (i = 0; i < penguin_number; i++) {
      penguin[i].pref_direction = -1;
      penguin[i].pref_climb = 0;
      penguin[i].hold = 0;
      __xpenguins_init_penguin(penguin+i);
      penguin[i].x_map = 0;
      penguin[i].y_map = 0;
      penguin[i].width_map = 1; /* So that the screen isn't completely */
      penguin[i].height_map = 1; /*    cleared at the start */
      penguin[i].mapped = 0;
    }
    /* Find out where the windows are - should be done 
     * just before beginning the event loop */
    ToonLocateWindows();
  }
  xpenguins_active = 1;
  return NULL;
}


void
xpenguins_ignorepopups(char yn)
{
  if (yn) {
    ToonConfigure(TOON_NOSOLIDPOPUPS);
  }
  else {
    ToonConfigure(TOON_SOLIDPOPUPS);
  }
  if (xpenguins_active) {
    ToonCalculateAssociations(penguin, penguin_number);
    ToonLocateWindows();
    ToonRelocateAssociated(penguin, penguin_number);
  }
}

void
xpenguins_set_number(int n)
{
  int i;
  if (xpenguins_active) {
    if (n > penguin_number) {
      int i;
      if (n > PENGUIN_MAX) {
	n = PENGUIN_MAX;
      }
      for (i = penguin_number; i < n; i++) {
	__xpenguins_init_penguin(penguin+i);			
	penguin[i].active = 1;
      }
      penguin_number = n;
    }
    else if (n < penguin_number) {
      if (n < 0) {
	n = 0;
      }

      for (i = n; i < penguin_number; i++) {
	ToonData *gdata = penguin_data[penguin[i].genus];
	if (penguin[i].active) {
	  if (xpenguins_blood && gdata[PENGUIN_EXIT].exists) {
	    ToonSetType(penguin+i, PENGUIN_EXIT,
			penguin[i].direction, TOON_DOWN);
	  }
	  else if (gdata[PENGUIN_EXPLOSION].exists) {
	    ToonSetType(penguin+i, PENGUIN_EXPLOSION,
			penguin[i].direction, TOON_HERE);
	  }
	  else {
	    penguin[i].active = 0;
	  }
	}
	penguin[i].terminating = 1;
      }

      ToonErase(penguin, penguin_number);
      ToonDraw(penguin, penguin_number);
      ToonFlush();
    }
  }
  else {
    penguin_number = n;
  }
  xpenguins_specify_number = 1;
}

/* Don't advance penguins, but check if they have been uncovered by 
 * moving windows and need to be redrawn. */
void
xpenguins_pause_frame()
{
  if (!xpenguins_active) {
    return;
  }

  /* check if windows have moved */
  if ( ToonWindowsMoved() ) {
    ToonErase(penguin, penguin_number);
    ToonDraw(penguin, penguin_number);
    ToonLocateWindows();
  }
}

/* Returns the number of penguins that are active or not terminating */
/* i.e. when 0 is returned, we can call xpenguins_exit() */
int
xpenguins_frame()
{
  int status, i, direction;
  int last_active = -1;

  if (!xpenguins_active) {
    return 0;
  }

  /*
   * Refresh the solid-window map when the X tree changes, and also on
   * a timer.  Modern compositors often do not deliver reliable
   * Unmap/Destroy to us for every closed client, so a periodic rescan
   * is required or penguins keep walking on vanished geometry.
   */
  {
    static int relocate_countdown = 0;
    int moved = ToonWindowsMoved();
    if (moved || --relocate_countdown <= 0) {
      relocate_countdown = 30; /* ~0.5–1s depending on theme delay */
      if (toon_debug && moved)
	fprintf(stderr, "[xpenguins-ng] window tree change -> relocate\n");
      else if (0 && toon_debug && !moved)
	fprintf(stderr, "[xpenguins-ng] periodic relocate\n");
      ToonCalculateAssociations(penguin, penguin_number);
      ToonLocateWindows();
      ToonRelocateAssociated(penguin, penguin_number);
    }
  }

  /* Loop through all the toons */
  for (i = 0; i < penguin_number; i++) {
    unsigned int type = penguin[i].type;
    ToonData *gdata = penguin_data[penguin[i].genus];
    if (!penguin[i].active) {
      if (!penguin[i].terminating) {
	__xpenguins_init_penguin(penguin+i);
	last_active = i;
      }
    }
    else if (toon_button_x >= 0
	     && type != PENGUIN_EXPLOSION && type != PENGUIN_ZAPPED
	     && type != PENGUIN_SQUASHED && type != PENGUIN_ANGEL
	     && type != PENGUIN_SPLATTED && type != PENGUIN_EXIT
	     && !penguin[i].terminating
	     && toon_button_x > penguin[i].x_map
	     && toon_button_y > penguin[i].y_map
	     && toon_button_x < penguin[i].x_map + penguin[i].width_map
	     && toon_button_y < penguin[i].y_map + penguin[i].height_map) {
      /* Toon has been hit by a button press */
      if (xpenguins_blood && gdata[PENGUIN_ZAPPED].exists) {
	ToonSetType(penguin+i, PENGUIN_ZAPPED,
		    penguin[i].direction, TOON_DOWN);
      }
      else if (gdata[PENGUIN_EXPLOSION].exists) {
	ToonSetType(penguin+i, PENGUIN_EXPLOSION,
		    penguin[i].direction, TOON_HERE);
      }
      else {
	penguin[i].active = 0;
      }
      ToonSetAssociation(penguin+i, TOON_UNASSOCIATED);
      last_active = i;
    }
    else {
      ToonData *data = gdata + type;
      long int conf = data->conf;

      last_active = i;
      /*
       * TOON_HERE means the toon is embedded in solid window area
       * (e.g. a window was mapped on top of it).  Fallers/tumblers/
       * floaters legitimately overlap the top edge of windows while
       * landing — especially maximized windows at y==0, since spawn
       * is at y = 1-height.  Their collisions are handled by
       * ToonAdvance; do not treat that as a squash.
       */
      if (type != PENGUIN_FALLER && type != PENGUIN_TUMBLER
	  && type != PENGUIN_FLOATER
	  && !((conf & TOON_NOBLOCK) | (conf & TOON_INVULNERABLE))
	  && ToonBlocked(penguin+i, TOON_HERE)) {
	if (xpenguins_blood && gdata[PENGUIN_SQUASHED].exists) {
	  ToonSetType(penguin+i, PENGUIN_SQUASHED,
		      penguin[i].direction, TOON_HERE);
	}
	else if (gdata[PENGUIN_EXPLOSION].exists) {
	  ToonSetType(penguin+i, PENGUIN_EXPLOSION,
		      penguin[i].direction, TOON_HERE);
	}
	else {
	  penguin[i].active = 0;
	}
	ToonSetVelocity(penguin+i, 0, 0);
	ToonSetAssociation(penguin+i, TOON_UNASSOCIATED);
      }
      else {
	status=ToonAdvance(penguin+i,TOON_MOVE);
	switch (type) {
	case PENGUIN_FALLER:
	  if (status != TOON_OK) {
	    if (ToonBlocked(penguin+i,TOON_DOWN)) {
	      if (penguin[i].pref_direction > -1)
		penguin[i].direction = penguin[i].pref_direction;
	      else
		penguin[i].direction = RandInt(2);
	      __xpenguins_make_walker(penguin+i, 0);
	      __xpenguins_lift_out_of_solid(penguin+i);
	      penguin[i].pref_direction = -1;
	    }
	    else {
	      if (!gdata[PENGUIN_CLIMBER].exists
		  || RandInt(2)) {
		ToonSetVelocity(penguin+i, -penguin[i].u,
				gdata[PENGUIN_FALLER].speed);
	      }
	      else {
		penguin[i].direction = (penguin[i].u > 0);
		__xpenguins_make_climber(penguin+i);
	      }
	    }
	  }
	  else if (penguin[i].v < data->terminal_velocity) {
	    penguin[i].v += data->acceleration;
	  }
	  break;

	case PENGUIN_TUMBLER:
	  if (status != TOON_OK) {
	    if (xpenguins_blood && data[PENGUIN_SPLATTED].exists
		&& penguin[i].v >= data->terminal_velocity
		&& !RandInt(3)) {
	      ToonSetType(penguin+i, PENGUIN_SPLATTED, TOON_LEFT, TOON_DOWN);
	      ToonSetAssociation(penguin+i, TOON_DOWN);
	      ToonSetVelocity(penguin+i, 0, 0);
	    }
	    else {
	      if (penguin[i].pref_direction > -1)
		penguin[i].direction = penguin[i].pref_direction;
	      else
		penguin[i].direction = RandInt(2);
	      __xpenguins_make_walker(penguin+i, 0);
	      __xpenguins_lift_out_of_solid(penguin+i);
	      penguin[i].pref_direction = -1;
	    }
	  }
	  else if (penguin[i].v < data->terminal_velocity) {
	    penguin[i].v += data->acceleration;
	  }
	  break;

	case PENGUIN_WALKER:
	case PENGUIN_RUNNER:
	  if (status != TOON_OK) {
	    if (status == TOON_BLOCKED) {
	      /* Try to step up... */
	      int u = penguin[i].u;
	      if (!ToonOffsetBlocked(penguin+i, u, -PENGUIN_JUMP)) {
		ToonMove(penguin+i, u, -PENGUIN_JUMP);
		ToonSetVelocity(penguin+i, 0, PENGUIN_JUMP - 1);
		ToonAdvance(penguin+i, TOON_MOVE);
		ToonSetVelocity(penguin+i, u, 0);
		/* Don't forget to accelerate! */
		if (abs(u) < data->terminal_velocity) {
		  if (penguin[i].direction) {
		    penguin[i].u += data->acceleration;
		  }
		  else {
		    penguin[i].u -= data->acceleration;
		  }
		}
	      }
	      else {
		/* Blocked! We can turn round, fly or climb... */
		int n = RandInt(8) * (1 - penguin[i].pref_climb);
		if (n < 2) {
		  char floater_exists = gdata[PENGUIN_FLOATER].exists;
		  char climber_exists = gdata[PENGUIN_CLIMBER].exists;
		  if ((n == 0 || !floater_exists) && climber_exists) {
		    __xpenguins_make_climber(penguin+i);
		    break;
		  }
		  else if (floater_exists) {
		    /* Make floater */
		    unsigned int newdir = !penguin[i].direction;
		    ToonSetType(penguin+i, PENGUIN_FLOATER,
				newdir, TOON_DOWN);
		    ToonSetAssociation(penguin+i, TOON_UNASSOCIATED);
		    ToonSetVelocity(penguin+i, 
				    (RandInt(5)+1) * (newdir*2-1),
				    -gdata[PENGUIN_FLOATER].speed);
		    break;
		  }
		}
		else {
		  /* Change direction *after* creating toon to make sure
                     that a runner doesn't get instantly squashed... */
		  __xpenguins_make_walker(penguin+i, 0);
		  penguin[i].direction = (!penguin[i].direction);
		  penguin[i].u = -penguin[i].u;
		}
	      }
	    }
	  }
	  else if (!ToonBlocked(penguin+i, TOON_DOWN)) {
	    /* Try to step down... */
	    int u = penguin[i].u; /* Save velocity */
	    ToonSetVelocity(penguin+i, 0, PENGUIN_JUMP);
	    status=ToonAdvance(penguin+i, TOON_MOVE);
	    if (status == TOON_OK) {
	      penguin[i].pref_direction = penguin[i].direction;
	      if (gdata[PENGUIN_TUMBLER].exists) {
		ToonSetType(penguin+i, PENGUIN_TUMBLER,
			    penguin[i].direction, TOON_DOWN);
		ToonSetAssociation(penguin+i, TOON_UNASSOCIATED);
		ToonSetVelocity(penguin+i, 0, gdata[PENGUIN_TUMBLER].speed);
	      }
	      else {
		__xpenguins_make_faller(penguin+i);
		penguin[i].u = 0;
	      }
	      penguin[i].pref_climb = 0;
	      
	    }
	    else {
	      ToonSetVelocity(penguin+i, u, 0);
	    }
	  }
	  else if (gdata[PENGUIN_ACTION0].exists && !RandInt(100)) {
	    unsigned int action = 1;
	    /* find out how many actions have been defined */
	    while (gdata[PENGUIN_ACTION0+action].exists
		   && ++action < PENGUIN_NACTIONS);
	    if (action) {
	      action = RandInt(action);
	    }
	    else {
	      action = 0;
	    }
	    /* If we have enough space, start the action: */
	    if (!ToonCheckBlocked(penguin+i, PENGUIN_ACTION0 + action, TOON_DOWN)) {
	      ToonSetType(penguin+i, PENGUIN_ACTION0 + action,
			  penguin[i].direction, TOON_DOWN);
	      ToonSetVelocity(penguin+i, gdata[PENGUIN_ACTION0+action].speed
			      * ((2*penguin[i].direction)-1), 0);
	    }
	  }
	  else if (abs(penguin[i].u) < data->terminal_velocity) {
	    if (penguin[i].direction) {
	      penguin[i].u += data->acceleration;
	    }
	    else {
	      penguin[i].u -= data->acceleration;
	    }
	  }
	  break;

	case PENGUIN_ACTION0:
	case PENGUIN_ACTION1:
	case PENGUIN_ACTION2:
	case PENGUIN_ACTION3:
	case PENGUIN_ACTION4:
	case PENGUIN_ACTION5:
	  if (status != TOON_OK) {
	    /* Try to drift up... */
	    int u = penguin[i].u;
	    if (!ToonOffsetBlocked(penguin+i, u, -PENGUIN_JUMP)) {
	      ToonMove(penguin+i, u, -PENGUIN_JUMP);
	      ToonSetVelocity(penguin+i, 0, PENGUIN_JUMP - 1);
	      ToonAdvance(penguin+i, TOON_MOVE);
	      ToonSetVelocity(penguin+i, u, 0);
	    }
	    else {
	      /* Blocked! Turn back into a walker: */
	      __xpenguins_make_walker(penguin+i, 0);
	    }
	  }
	  else if (!ToonBlocked(penguin+i, TOON_DOWN)) {
	    /* Try to drift down... */
	    ToonSetVelocity(penguin+i, 0, PENGUIN_JUMP);
	    status=ToonAdvance(penguin+i, TOON_MOVE);
	    if (status == TOON_OK) {
	      if (gdata[PENGUIN_TUMBLER].exists) {
		ToonSetType(penguin+i, PENGUIN_TUMBLER,
			    penguin[i].direction, TOON_DOWN);
		ToonSetAssociation(penguin+i, TOON_UNASSOCIATED);
		ToonSetVelocity(penguin+i, 0,
				gdata[PENGUIN_TUMBLER].speed);
	      }
	      else {
		__xpenguins_make_faller(penguin+i);
	      }
	      penguin[i].pref_climb = 0;
	    }
	    else {
	      ToonSetVelocity(penguin+i, data->speed
			      * ((2*penguin[i].direction)-1), 0);
	    }
	  }
	  else if (penguin[i].frame == 0) {
	    int loop = data->loop;
	    if (!loop) {
	      loop = -10;
	    }
	    if (loop < 0) {
	      if (!RandInt(-loop)) {
		__xpenguins_make_walker(penguin+i, 0);
	      }
	    }
	    else if (penguin[i].cycle >= loop) {
	      __xpenguins_make_walker(penguin+i, 0);
	    }
	  }
	  break;

	case PENGUIN_CLIMBER:
	  direction = penguin[i].direction;
	  if (penguin[i].y < 0) {
	    penguin[i].direction = (!direction);
	    __xpenguins_make_faller(penguin+i);
	    penguin[i].pref_climb = 0;
	  }
	  else if (status == TOON_BLOCKED) {
	    /* Try to step out... */
	    int v = penguin[i].v;
	    int xoffset = (1-direction*2) * PENGUIN_JUMP;
	    if (!ToonOffsetBlocked(penguin+i, xoffset, v)) {
	      ToonMove(penguin+i, xoffset, v);
	      ToonSetVelocity(penguin+i, -xoffset-(1-direction*2), 0);
	      ToonAdvance(penguin+i, TOON_MOVE);
	      ToonSetVelocity(penguin+i, 0, v);
	    }
	    else {
	      penguin[i].direction = (!direction);
	      __xpenguins_make_faller(penguin+i);
	      penguin[i].pref_climb = 0;
	    }
	  }
	  else if (!ToonBlocked(penguin+i, direction)) {
	    if (ToonOffsetBlocked(penguin+i, ((2*direction)-1)
				  * PENGUIN_JUMP, 0)) {
	      ToonSetVelocity(penguin+i, ((2*direction)-1)
			      * (PENGUIN_JUMP - 1), 0);
	      ToonAdvance(penguin+i, TOON_MOVE);
	      ToonSetVelocity(penguin+i, 0, -data->speed);
	    }
	    else {
	      __xpenguins_make_walker(penguin+i, 1);
	      ToonSetPosition(penguin+i, penguin[i].x + (2*direction)-1,
			      penguin[i].y);
	      penguin[i].pref_direction = direction;
	      penguin[i].pref_climb = 1;
	    }
	  }
	  else if (penguin[i].v > -data->terminal_velocity) {
	    penguin[i].v -= data->acceleration;
	  }
	  break;

	case PENGUIN_FLOATER:
	  if (penguin[i].y < 0) {
	    penguin[i].direction = (penguin[i].u > 0);
	    __xpenguins_make_faller(penguin+i);
	  }
	  else if (status != TOON_OK) {
	    if (ToonBlocked(penguin+i, TOON_UP)) {
	      penguin[i].direction = (penguin[i].u>0);
	      __xpenguins_make_faller(penguin+i);
	    }
	    else {
	      penguin[i].direction = !penguin[i].direction;
	      ToonSetVelocity(penguin+i,-penguin[i].u,
			      -data->speed);
	    }
	  }
	  break;
	case PENGUIN_EXPLOSION:
	  if (xpenguins_angels && !penguin[i].terminating
	     && gdata[PENGUIN_ANGEL].exists) {
	    ToonSetType(penguin+i, PENGUIN_ANGEL,
			penguin[i].direction, TOON_HERE);
	    ToonSetVelocity(penguin+i, RandInt(5) -2,
			    -gdata[PENGUIN_ANGEL].speed);
	    ToonSetAssociation(penguin+i, TOON_UNASSOCIATED);
	  }
	case PENGUIN_ANGEL:
	  if (penguin[i].y < -((int) data->height)) {
	    penguin[i].active = 0;
	  }
	  if (status != TOON_OK) {
	    penguin[i].u = -penguin[i].u;
	  } 
	}
      }
    }
  }
  /* First erase them all, then draw them all
   * - greatly reduces flickering */
  ToonErase(penguin, penguin_number);
  ToonDraw(penguin, penguin_number);
  ToonFlush();

  penguin_number = last_active + 1;

  /* Clear any button press information */
  toon_button_x = toon_button_y = -1;

  return penguin_number;
}

/* Erase all penguins and close the display */
void
xpenguins_exit()
{
  ToonCloseDisplay();
  xpenguins_active = 0;
}

/* Exit sequence... */
void
xpenguins_exit_sequence(unsigned int sleep_msec) {
  unsigned long penguin_sleep_usec = sleep_msec*1000;
  if (!xpenguins_active) {
    return;
  }
  xpenguins_terminate();
  while(xpenguins_frame()) {
    xpenguins_sleep(penguin_sleep_usec);
  }
  xpenguins_exit();
}


/* toon_init.c - initialising various things
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
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <X11/cursorfont.h>
#include "toon.h"


/* STARTUP FUNCTIONS */

/* Open display */
Display *
ToonOpenDisplay(char *display_name)
{
  toon_display=XOpenDisplay(display_name);
  if (toon_display == NULL) {
    if (display_name == NULL && getenv("DISPLAY") == NULL)
      strncpy(toon_error_message, _("DISPLAY environment variable not set"),
	      TOON_MESSAGE_LENGTH);
    else
      strncpy(toon_error_message, _("Can't open display"),
	      TOON_MESSAGE_LENGTH);
    return(NULL);
  }
  ToonInit(toon_display);
  return toon_display;
}

/* Setup graphics context and create some XRegions */
/* Currently this function always returns 0 */
int
ToonInit(Display *d)
{
  int screen = 0;
  XGCValues gc_values;
  XWindowAttributes attributes;

  toon_display = d;

  screen = DefaultScreen(toon_display);

  *toon_message = '\0';
  if (toon_root_override) {
    toon_root = toon_parent = toon_root_override;
  }
  else {
    toon_root = ToonGetRootWindow(toon_display, screen, &toon_parent);
  }
  XGetWindowAttributes(toon_display, toon_root, &attributes);
  toon_display_width = attributes.width;
  toon_display_height = attributes.height;
  if (toon_root != toon_parent) {
    /* Work out the position of toon_root with respect to toon_parent;
     * assume for now that toon_parent is the same size as the root
     * window */
    toon_x_offset = attributes.x;
    toon_y_offset = attributes.y;
  }

  /* If we want to squish the toons with the mouse then we must create
   * a window over the root window that has the same properties */
  if (toon_squish) {
    XSetWindowAttributes squish_att;
    squish_att.event_mask = ButtonPressMask;
    squish_att.override_redirect = True;
    toon_squish_window = XCreateWindow(toon_display, toon_root, 0, 0,
				       toon_display_width, toon_display_height,
				       0, CopyFromParent, InputOnly, CopyFromParent,
				       CWOverrideRedirect | CWEventMask,
				       &squish_att);
    XDefineCursor(toon_display, toon_squish_window,
		  XCreateFontCursor(toon_display, XC_target));
    XLowerWindow(toon_display, toon_squish_window);
  }

  /* Is anyone interested in this window? If so we must inform them of
   * where the toons are by sending expose events - that way they can
   * redraw themselves when a toon walks over them */
  if (attributes.all_event_masks & ExposureMask) {
    int len = strlen(toon_message);
    toon_expose = 1;
    if (*toon_message) {
      snprintf(toon_message + len, TOON_MESSAGE_LENGTH - len,
	       _(" and redrawing overwritten desktop icons"));
    }
    else {
      snprintf(toon_message, TOON_MESSAGE_LENGTH,
	       _("Redrawing overwritten desktop icons"));
    }
    toon_message[TOON_MESSAGE_LENGTH-1] = '\0';
  }
  else {
    toon_expose = 0;
  }
  
  /* Set Graphics Context */
  gc_values.function = GXcopy;
  gc_values.graphics_exposures = False;
  gc_values.fill_style = FillTiled;
  toon_drawGC = XCreateGC(toon_display, toon_root,
			  GCFunction | GCFillStyle | GCGraphicsExposures,
			  &gc_values);

  /* Regions */
  toon_windows = XCreateRegion();

  /* Notify if the location of the client windows changes,
     or if the window we are drawing to changes size */
  if (toon_root != RootWindow(toon_display, screen)) {
    if (toon_root == toon_parent) {
      XSelectInput(toon_display, toon_root, SubstructureNotifyMask
		   | StructureNotifyMask);
    }
    else {
      XSelectInput(toon_display, toon_root, StructureNotifyMask);
      XSelectInput(toon_display, toon_parent, SubstructureNotifyMask);
    }
  }
  else {
    XSelectInput(toon_display, toon_parent, SubstructureNotifyMask);
  }

  toon_nwindows = 0;
  
  if (toon_squish_window) {
    XMapWindow(toon_display, toon_squish_window);
  }

  return 0;
}

/* Configure signal handling and the way the toons behave via a bitmask */
/* Currently always returns 0 */
int
ToonConfigure(unsigned long int code)
{
  if (code & TOON_EDGEBLOCK)
    toon_edge_block=1;
  else if (code & TOON_SIDEBOTTOMBLOCK)
    toon_edge_block=2;
  else if (code & TOON_NOEDGEBLOCK)
    toon_edge_block=0;

  if (code & TOON_SOLIDPOPUPS)
    toon_solid_popups=1;
  else if (code & TOON_NOSOLIDPOPUPS)
    toon_solid_popups=0;

  if (code & TOON_SHAPEDWINDOWS)
    toon_shaped_windows=1;
  else if (code & TOON_NOSHAPEDWINDOWS)
    toon_shaped_windows=0;

  if (code & TOON_CATCHSIGNALS) {
    signal(SIGINT, __ToonSignalHandler);
    signal(SIGTERM, __ToonSignalHandler);
    signal(SIGHUP, __ToonSignalHandler);
  }
  else if (code & TOON_EXITGRACEFULLY) {
    signal(SIGINT, __ToonExitGracefully);
    signal(SIGTERM, __ToonExitGracefully);
    signal(SIGHUP, __ToonExitGracefully);
  }
  else if (code & TOON_NOCATCHSIGNALS) {
    signal(SIGINT, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGHUP, SIG_DFL);
  }

  if (code & TOON_SQUISH) {
    toon_squish = 1;
  }
  else if (code & TOON_NOSQUISH) {
    toon_squish = 0;
  }

  return 0;
}

/* Store the pixmaps to the server */
/* Returns 0 on success, otherwise the return value from the Xpm function */
int
ToonInstallData(ToonData **data, int ngenera, int ntypes)
{
  int i, j, status;
  XpmAttributes attributes;
  attributes.valuemask = (XpmReturnPixels
			  | XpmReturnExtensions | XpmExactColors 
			  | XpmCloseness);
  attributes.exactColors=False;
  attributes.closeness=40000;
  for (i = 0; i < ngenera; ++i) {
    for (j = 0; j < ntypes; ++j) {
      ToonData *d = data[i]+j;
      if (d->exists && !d->master) {
	if ((status =
	     XpmCreatePixmapFromData(toon_display, toon_root,
				     d->image,
				     &(d->pixmap), 
				     &(d->mask), 
				     &attributes))) {
	  return status;
	}
      }
    }
    /* Loop through the types again for any pixmaps that are copies */
    for (j = 0; j < ntypes; ++j) {
      ToonData *d = data[i]+j;
      if (d->exists && d->master) {
	d->pixmap = d->master->pixmap;
	d->mask = d->master->mask;
      }
    }
  }

  toon_data = data;
  toon_ngenera = ngenera;
  toon_ntypes = ntypes;
  return 0;
}

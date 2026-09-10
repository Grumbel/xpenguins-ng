/* SPDX-FileCopyrightText: 1999-2001 Robin Hogan
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* xpenguins.h - functions for animating penguins in your root window
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

#define PENGUIN_MAX 256

#define PENGUIN_DEFAULTWIDTH 30
#define PENGUIN_DEFAULTHEIGHT 30

#define PENGUIN_FORWARD 0
#define PENGUIN_LEFTRIGHT 1
#define PENGUIN_LEFT 0
#define PENGUIN_RIGHT 1

#define PENGUIN_WALKER 0    /* Walking along the tops of windows */
#define PENGUIN_FALLER 1    /* Falling from the top of the screen */
#define PENGUIN_TUMBLER 2   /* After falling off a window */
#define PENGUIN_FLOATER 3   /* Well, flying really */
#define PENGUIN_CLIMBER 4   /* Climbing up the sides of windows or the screen */
#define PENGUIN_EXIT 5      /* Exit sequence */
#define PENGUIN_EXPLOSION 6 /* Simple explosion with NO BLOOD! */
#define PENGUIN_RUNNER 7    /* Just like walker but usually faster */
#define PENGUIN_SPLATTED 8  /* Sometimes when falling onto a hard surface */
#define PENGUIN_SQUASHED 9  /* When caught under windows */
#define PENGUIN_ZAPPED 10   /* Zapped by mouse pointer (not yet used) */
#define PENGUIN_ANGEL 11    /* After some nasty death, floating upwards */
#define PENGUIN_ACTION0 12  /* Reading, sleeping, jumping, whatever */
#define PENGUIN_ACTION1 13  /* The ACTIONs must have consecutive numbers */
#define PENGUIN_ACTION2 14
#define PENGUIN_ACTION3 15
#define PENGUIN_ACTION4 16
#define PENGUIN_ACTION5 17

#define PENGUIN_NGENERA 2   /* Number of genera allocated, not the max possible */
#define PENGUIN_NTYPES 18
#define PENGUIN_NACTIONS 6

#define PENGUIN_JUMP 8

#define PENGUIN_ARTIST 1
#define PENGUIN_MAINTAINER 2
#define PENGUIN_DATE 3
#define PENGUIN_COPYRIGHT 4
#define PENGUIN_LICENSE 5
#define PENGUIN_ICON 6
#define PENGUIN_COMMENT 7

#define PENGUIN_NABOUTFIELDS 8

#define XPENGUINS_DEBUG fprintf(stderr, __FILE__ ": line %d\n", __LINE__)

/*
 * Themes are searched for in the following locations:
 * $HOME/.xpenguins/themes
 * [xpenguins_directory]/themes
 */
#ifndef XPENGUINS_SYSTEM_DIRECTORY
#ifdef PKGDATADIR
#define XPENGUINS_SYSTEM_DIRECTORY PKGDATADIR
#else
#define XPENGUINS_SYSTEM_DIRECTORY "/usr/share/xpenguins-ng"
#endif

#endif
#ifndef XPENGUINS_THEME_DIRECTORY
#define XPENGUINS_THEME_DIRECTORY "/themes"
#endif

#ifndef XPENGUINS_CONFIG
#define XPENGUINS_CONFIG "/config"
#endif

/* The XPenguinsTheme structure contains all the information about the toon,
 *  basically an array of ToonData structures */

typedef struct {
  ToonData **data;
  char **name;
  unsigned int *number;
  unsigned int total; /* Sum of "number", but not more than PENGUIN_MAX */
  unsigned int ngenera;
  unsigned int delay;
  unsigned int _nallocated;
} XPenguinsTheme;

extern char *xpenguins_directory;
extern ToonData **penguin_data;
extern unsigned int penguin_ngenera;
extern unsigned int *penguin_numbers;
extern int penguin_number;
extern char xpenguins_verbose;
extern char xpenguins_blood;
extern char xpenguins_angels;

/*** FUNCTIONS in xpenguins_core.c ***/

/* Contact X server and upload the data in `penguin_data'.
 * On success NULL is returned, otherwise a pointer to a static
 * message indicating the fault is returned.  */
char *xpenguins_start(char *display_name);

/* Set the required number of toons to animate. If the toons are
 * active and the number is less than the current number then the
 * excess will be turned into exiters or explosions and their
 * terminating flag will be set. */
void xpenguins_set_number(int n);

/* Ignore or observe `popup' windows */
void xpenguins_ignorepopups(char yn);

/* Advance the toons by one frame. Returns the number of penguins that
 * are active or not terminating i.e. when 0 is returned, we can call
 * xpenguins_exit(). */
int xpenguins_frame();

/* Don't advance penguins, but check if they have been uncovered by 
 * moving windows and need to be redrawn. */
void xpenguins_pause_frame();

/* The command-line version of xpenguins will want to call this to do
 * the delays between frames, while the GNOME version will use its own
 * callbacks. */
#define xpenguins_sleep ToonSleep

/* Turn all the penguins into exiters and set their terminating flag */
#define xpenguins_terminate() xpenguins_set_number(0)

/* Erase all penguins and close the display */
void xpenguins_exit();


/*** FUNCTIONS in xpenguins_themes.c ***/

/* Set the data directory to use to find themes */
void xpenguins_set_directory(char *directory);
/* XDG data dir for user themes (~/.local/share/xpenguins-ng) */
const char *xpenguins_user_data_dir(void);
int xpenguins_tray_init(Display *dpy);
/* Tray event hook: 0=ignore, 1=exit, 2=cycle theme */
int xpenguins_tray_event(XEvent *event);
/* Switch to the next installed theme; updates theme and server pixmaps. */
char *xpenguins_cycle_theme(XPenguinsTheme *theme);
void xpenguins_set_theme_list_name(const char *name);
void xpenguins_tray_poll(void);
void xpenguins_tray_fini(void);


/* Returns an array of strings the theme names - this list is
 * dynamically allocated so should be freed using
 * xpenguins_free_list(), unless NULL is returned, in which case no
 * themes were found. The number of themes is returned in n. */
char **xpenguins_list_themes(int *n);

/* Generic function for freeing character lists that are allocated for
 * verious purposes. */
void xpenguins_free_list(char **list);

/* Returns a string containing the directory where the theme called
 * `name' can be found - this string should be freed using free(),
 * unless NULL is returned, which indicates that the requested theme
 * could not be found.  */
char *xpenguins_theme_directory(char *name);

/* Loads a theme into a pre-existing structure, returns 0 on success,
 * a (statically-allocated) string containing the error on failure. */
char *xpenguins_load_theme(char *name, XPenguinsTheme *theme);

char *xpenguins_load_themes(char **names, XPenguinsTheme *theme);

/* Free all the data associated with a theme */
void xpenguins_free_theme(XPenguinsTheme *theme);

/* Print basic theme information to standard error - for debugging
 * purposes */
void xpenguins_describe_theme(XPenguinsTheme *theme);

/* Read the `about' file in a theme directory and return a string list
 * of theme properties. This list should be freed with
 * xpenguins_free_list(). */
char **xpenguins_theme_info(char *name);

/* Remove underscores from a theme name */
char *xpenguins_remove_underscores(char *themename);

/* Don't show any nasty deaths */
#define xpenguins_set_blood(yn) xpenguins_blood = yn

/* Don't show angels flying up to heaven */
#define xpenguins_set_angels(yn) xpenguins_angels = yn


/*** FUNCTIONS in xpenguins_config.c ***/

/* Read the next word from a config file into buf */
int xpenguins_read_word(FILE *file, int max_size, char *buf);

/* Read a line from a config file */
int xpenguins_read_line(FILE *file, int max_size, char *buf);

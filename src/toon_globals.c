/* toon_globals.c - declare and initialise global variables
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

Display *toon_display;
Window toon_root;
Window toon_parent;
Window toon_draw_window = 0;
Window toon_root_override = 0;
/* 0 = classic root drawing, 1 = transparent overlay */
char toon_overlay_mode = 0;
/* -1 = auto, 0 = force classic, 1 = force overlay */
char toon_overlay_preference = -1;
int toon_x_offset = 0;
int toon_y_offset = 0;
int toon_display_width, toon_display_height;
GC toon_drawGC = NULL;
Region toon_windows = NULL;
unsigned int toon_nwindows = 0;

__ToonWindowData *toon_windata = NULL;
ToonData **toon_data = NULL;
int toon_ngenera = 0;
int toon_ntypes = 0;
int toon_errno = 0;
/* Do the edges block movement?
 * If only the sides and the bottom block movement then toon_edge_block = 2 */
char toon_edge_block = 0;
char toon_shaped_windows = 1;
/* If solid_popups is set to 0 then the toons fall behind `popup' windows.
 * This includes the KDE panel */
char toon_solid_popups = 1;
int toon_signal = 0;
char toon_error_message[TOON_MESSAGE_LENGTH] = "";
char toon_message[TOON_MESSAGE_LENGTH] = "";
int toon_max_relocate_up = TOON_DEFAULTMAXRELOCATE;
int toon_max_relocate_down = TOON_DEFAULTMAXRELOCATE;
int toon_max_relocate_left = TOON_DEFAULTMAXRELOCATE;
int toon_max_relocate_right = TOON_DEFAULTMAXRELOCATE;
int toon_button_x = -1;
int toon_button_y = -1;
char toon_expose;
char toon_squish = 0;
Window toon_squish_window = (Window) 0;

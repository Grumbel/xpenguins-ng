/* SPDX-FileCopyrightText: 1999-2001 Robin Hogan
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* toon_signal.c - signal and error handling functions
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
#include "toon.h"

/* SIGNAL AND ERROR HANDLING FUNCTIONS */

/* Signal Handler: stores caught signal in toon_signal */
void
__ToonSignalHandler(int sig)
{
  toon_signal=sig;
  return;
}


/* Clear root window, close display and exit  */
void
__ToonExitGracefully(int sig)
{
  ToonConfigure(TOON_NOCATCHSIGNALS);
  ToonCloseDisplay();
  exit(0);
}

/* Has a signal been caught? If so return it */
int
ToonSignal()
{
  int sig = toon_signal;
  toon_signal = 0;
  return sig;
}

/* Return error message */
char *
ToonErrorMessage()
{
  return toon_error_message;
}

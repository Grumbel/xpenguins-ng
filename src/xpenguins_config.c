/* SPDX-FileCopyrightText: 1999-2001 Robin Hogan
 * SPDX-FileCopyrightText: 2026 xpenguins-ng contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* xpenguins_config.c - simple functions for reading a config file
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

/* 
 * Read the next word from a file into buf,
 * returning the number of characters read.
 * A return value of 0 indicates end of file,
 * and -1 indicates the word was longer than max_size.
 * Whitespace and #'ed comments are skipped.
 */
int
xpenguins_read_word(FILE *file, int max_size, char *buf)
{
  int ch;
  int buf_size = 0;
  /* Skip white space and comments */
  while (1) {
    ch = fgetc(file);
    if (ch == EOF) {
      return 0;
    }
    else if (ch == '#') {
      /* Skip line */
      while (ch != '\n') {
	if ((ch = fgetc(file)) == EOF) {
	  return 0;
	}
      }
    }
    else if (ch > ' ') {
      break;
    }
  }
  /* Found start of word */
  while (ch > ' ' && ch != '#' && ch != EOF) {
    if (buf_size < max_size-1) {
      buf[buf_size++] = ch;
    }
    else {
      /* Word longer than max_size */
      buf[buf_size] = '\0';
      return -1;
    }
    ch = fgetc(file);
  }
  ungetc(ch, file);
  buf[buf_size] = '\0';
  return buf_size;
}

/*
 * Read from the current position to the end of the line,
 * returning the number of characters read.
 * Leading and trailing whitespace is ignored.
 * A return value of -1 indicates that
 * the string was longer than max_size,
 * in which case the remainter of the line will be discarded.
 */
int
xpenguins_read_line(FILE *file, int max_size, char *buf)
{
  int ch;
  int buf_size = 0;
  int last_nonwhite = 0;
  /* Skip whitespace */
  while (1) {
    ch = fgetc(file);
    if (ch == '\n' || ch == EOF) {
      *buf = '\0';
      return 0;
    }
    else if (ch > ' ') {
      break;
    }
  }
  while (ch != '\n' && ch != EOF) {
    if (buf_size < max_size-1) {
      buf[buf_size++] = ch;
      if (ch != ' ' && ch != '\t') {
	last_nonwhite = buf_size;
      }
    }
    else {
      /* Line longer than max_size */
      buf[buf_size] = '\0';
      /* Flush line */
      while (ch != '\n' && ch != EOF) {
	ch = fgetc(file);
      }
      return buf_size;
    }
    ch = fgetc(file);
  }
  buf[last_nonwhite] = '\0';
  return last_nonwhite;
}

/* XPENGUINS - cool little penguins that walk along the tops of your windows
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
 *
 *
 * XPENGUINS is a kind of a cross between lemmings and xsnow - penguins
 * fall from the top of the screen and walk across the tops of windows.
 *
 * Since version 2 the code has been modularised - all the work is
 * done by the xpenguins_*() functions in libxpenguins. This file
 * contains the command-line interface. A GNOME applet interface also
 * exists. The main additional feature since version 2 is the
 * "themeability" (hey, everyone else is doing it so why shouldn't
 * I?) - images are loaded at run-time rather than being compiled into
 * the executable. This makes it relatively easy for artistically
 * inclined non-programmers to replace the penguins with anything they
 * choose. It also means that the images need not be licensed under
 * the GPL.
 * 
 * Robin Hogan <R.J.Hogan@reading.ac.uk>
 * Project started 22 November 1999
 * This file started 7 April 2001
 * See XPENGUINS_DATE for last update
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#else
#define VERSION "<unknown version>"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "xpenguins.h"

#define DEFAULT_THEME "Penguins"

#define XPENGUINS_VERSION VERSION
#define XPENGUINS_AUTHOR "Robin Hogan"
#define XPENGUINS_DATE "1 October 2001"

#define ArgumentIs(short_arg, long_arg) (strcmp(argv[n], short_arg) == 0 \
   || strcmp(argv[n], long_arg) == 0 || strcmp(argv[n], "-" long_arg) == 0)
#define LongArgumentIs(long_arg) \
   (strcmp(argv[n], long_arg) == 0 || strcmp(argv[n], "-" long_arg) == 0)

/* Prototypes */
void ShowUsage(char **argv);
void ListThemes();
void DescribeThemes(char **theme_names);
float LoadAverage();

XPenguinsTheme theme;
char *config_dir = NULL;

int
main (int argc, char **argv)
{
  char **theme_names = NULL;
  char *display_name = NULL;
  char *error_message = NULL;
  int n;
  int npenguins = -1; /* number of penguins (0 = use default) */
  unsigned int sleep_usec = 0;
  unsigned int load_check_interval = 5000000; /* 5s between load average checks */
  unsigned int load_cycles; /* number of frames between load average checks */
  unsigned int cycle = 0;
  unsigned int nthemes = 0;
  int interupts = 0;
  int frames_active;
  float load1 = -1.0; /* Start killing penguins if load reaches this amount */
  float load2 = -1.0; /* All gone by this amount (but can come back!) */
  /* Flags */
  char list_themes = 0;
  char describe_theme = 0;
  char ignore_popups = 0;
  char load_message = 0;
  char all_themes = 0;

  theme_names = malloc(2 * sizeof(char *));
  theme_names[0] = DEFAULT_THEME;
  theme_names[1] = NULL;

  /* Handle command-line arguments */
  for (n = 1; n < argc; ++n) {
    if (ArgumentIs("-n", "-penguins")) {
      if (argc > ++n) {
        npenguins = atoi(argv[n]);
	if (npenguins > PENGUIN_MAX) {
	  npenguins = PENGUIN_MAX;
	  if (xpenguins_verbose) {
	    fprintf(stderr, _("Warning: only %d penguins created\n"), npenguins);
	  }
	}
	else if (npenguins < 0) {
	  if (xpenguins_verbose) {
	    fprintf(stderr, _("Warning: using default number of penguins\n"));
	  }
	  npenguins = -1;
	}
      }
      else {
	fprintf(stderr, _("Error: number of penguins not specified\n"));
	ShowUsage(argv);
	exit(1);
      }
    }
    else if (ArgumentIs("-m", "-delay")) {
      if (argc > ++n) {
	sleep_usec=1000*atoi(argv[n]);
      }
      else {
	fprintf(stderr, _("Error: delay in milliseconds not specified\n"));
	ShowUsage(argv);
	exit(1);
      }
    }
    else if (ArgumentIs("-d", "-display")) {
      if (argc > ++n) {
	display_name = argv[n];
      }
      else {
	fprintf(stderr, _("Error: display not specified\n"));
	ShowUsage(argv);
	exit(1);
      }
    }
    else if (ArgumentIs("-p", "-ignorepopups")) {
      ignore_popups = 1;
    }
    else if (ArgumentIs("-r", "-rectwin")) {
      ToonConfigure(TOON_NOSHAPEDWINDOWS);
    }
    else if (ArgumentIs("-h", "-help")) {
      fprintf(stdout, _("XPenguins %s (%s) by %s\n"),
	      XPENGUINS_VERSION, XPENGUINS_DATE, XPENGUINS_AUTHOR);
      ShowUsage(argv);
      exit(0);
    }
    else if (ArgumentIs("-v", "-version")) {
      fprintf(stdout, "XPenguins %s\n", XPENGUINS_VERSION);
      exit(0);
    }
    else if (ArgumentIs("-q", "-quiet")) {
      xpenguins_verbose = 0;
    }
    else if (ArgumentIs("-c", "-config-dir")) {
      if (argc > ++n) {
	config_dir = argv[n];
      }
      else {
	fprintf(stderr, _("Error: config directory not specified\n"));
	ShowUsage(argv);
	exit(1);
      }
    }
    else if (ArgumentIs("-t", "-theme")) {
      if (argc > ++n) {
	if (nthemes) {
	  theme_names = realloc(theme_names, (nthemes+2) * sizeof(char **));
	  theme_names[nthemes] = argv[n]; /* Add a new theme to the list */
	  theme_names[++nthemes] = NULL;  /* Make sure the list is NULL terminated */
	}
	else {
	  *theme_names = argv[n]; /* Replace the default theme */
	  ++nthemes;
	}
      }
      else {
	fprintf(stderr, _("Error: theme not specified\n"));
	ShowUsage(argv);
	exit(1);
      }
    }
    else if (ArgumentIs("-l", "-list-themes")) {
      list_themes = 1;
    }
    else if (ArgumentIs("-i", "-theme-info")) {
      describe_theme = 1;
    }
    else if (ArgumentIs("-b", "-no-blood")) {
      xpenguins_set_blood(0);
    }
    else if (ArgumentIs("-a", "-no-angels")) {
      xpenguins_set_angels(0);
    }
    else if (ArgumentIs("-s", "-squish")) {
      ToonConfigure(TOON_SQUISH);
    }
    else if (LongArgumentIs("-debug")) {
      toon_debug = 1;
    }
    else if (LongArgumentIs("-overlay")) {
      ToonConfigure(TOON_OVERLAY);
    }
    else if (LongArgumentIs("-no-overlay") || LongArgumentIs("-root")) {
      ToonConfigure(TOON_NOOVERLAY);
    }
    else if (LongArgumentIs("-all")) {
      all_themes = 1;
    }
    else if (LongArgumentIs("-id")) {
      if (argc > ++n) {
	Window id;
	if (sscanf(argv[n], "%lu", &id) == 1) {
	  ToonSetRoot(id);
	}
	else {
	  fprintf(stderr, _("Warning: could not read window ID\n"));
	}
      }
      else {
	fprintf(stderr, _("Error: window ID not specified\n"));
	ShowUsage(argv);
	exit(1);
      }
    }
    else if (LongArgumentIs("-nice")) {
      if (argc > ++n) {
	if ((load1 = atof(argv[n])) < 0.0) {
	  fprintf(stderr, _("Error: load average must be greater than or equal to zero\n"));
	  ShowUsage(argv);
	  exit(1);
	}
	if (argc > ++n) {
	  if ((load2 = atof(argv[n])) < load1) {
	    fprintf(stderr, _("Error: second load average must be greater than or equal to first\n"));
	    ShowUsage(argv);
	    exit(1);
	  }
	}
	else {
	  fprintf(stderr, _("Error: second load average not specified\n"));
	  ShowUsage(argv);
	  exit(1);
	}
      }
      else {
	fprintf(stderr, _("Error: no load averages not specified\n"));
	ShowUsage(argv);
	exit(1);
      }
    }
    else {
      if (xpenguins_verbose) {
	fprintf(stderr, _("Warning: \"%s\" not understood - use \"-h\" for "
		"a list of options\n"), argv[n]);
      }
    } 
  }

  /* Some basic stuff */

  if (config_dir) {
    xpenguins_set_directory(config_dir);
  }

  if (all_themes) {
    char **tmp_list = xpenguins_list_themes(NULL);
    if (tmp_list) {
      free(theme_names);
      theme_names = tmp_list;
    }
  }

  if (list_themes) {
    ListThemes();
  }
  else if (describe_theme) {
    DescribeThemes(theme_names);
  }

  if (ignore_popups) {
    xpenguins_ignorepopups(1);
  }

  if (npenguins >= 0) {
    xpenguins_set_number(npenguins);
  }

  /* Load theme */
  error_message = xpenguins_load_themes(theme_names, &theme);
  if (error_message) {
    if (theme_names[1]) {
      fprintf(stderr, _("Error loading themes: %s\n"), error_message);
    }
    else {
      fprintf(stderr, _("Error loading theme %s: %s\n"), *theme_names,
	      error_message);
    }
    exit(2);
  }
  free(theme_names);

  if (!sleep_usec) {
    sleep_usec = 1000.0 * theme.delay;
  }
    
  if (load1 >= 0.0) {
    float load = LoadAverage();
    if (load < 0.0) {
      fprintf(stderr, _("Warning: cannot detect load averages on this system\n"));
      load1 = load2 = -1.0;
    }
    else {
      load_cycles = load_check_interval / sleep_usec;
    }
  }

  /* Send pixmaps to X server */
  error_message = xpenguins_start(display_name);
  if (error_message) {
    fprintf(stderr, _("Error starting xpenguins: %s\n"),
	    error_message);
    exit(2);
  }

  /* We want npenguins to represent the full complement of penguins;
   * penguin_number may change if the load gets too high */
  if (npenguins <= 0) {
    npenguins = penguin_number;
  }

  ToonConfigure(TOON_CATCHSIGNALS);

  /* Main loop */
  while((frames_active = xpenguins_frame()) || !interupts) {
   if (interupts && xpenguins_verbose) {
      fprintf(stderr, ".");
    }
    if (ToonSignal()) {
      if (++interupts > 1) {
	break;
      }
      else if (xpenguins_verbose) {
	fprintf(stderr, _("Interupt received: Exiting."));
      }
      ToonConfigure(TOON_EXITGRACEFULLY);
      xpenguins_set_number(0);
    }
    else if (!interupts && cycle > load_cycles && load1 >= 0.0) {
      float load = LoadAverage();
      int newp;

      if (load2 > load1) {
	newp = ((load2-load)*((float) npenguins)) / (load2-load1);
	if (newp > npenguins) {
	  newp = npenguins;
	}
	else if (newp < 0) {
	  newp = 0;
	}
      }
      else if (load < load1) {
	newp = npenguins;
      }
      else {
	newp = 0;
      }

      if (penguin_number != newp) {
	xpenguins_set_number(newp);
	if (xpenguins_verbose && !load_message) {
	  fprintf(stderr, _("Adjusting number according to load\n"));
	  load_message = 1;
	}
      }
      cycle = 0;
    }
    else if (!frames_active) {
      /* No frames active! Hybernate for 5 seconds... */
      xpenguins_sleep(load_check_interval);
      cycle = load_cycles;
    }
    xpenguins_sleep(sleep_usec);
    ++cycle;
  }
  if (xpenguins_verbose) {
    fprintf(stderr, _(" Done.\n"));
  }
  xpenguins_exit();
  exit(0);
}


void
ShowUsage(char **argv)
{
   printf(_("Usage: %s [options]\n"), argv[0]);
   printf(_("Options:\n"
	    "  -d, --display        <display>    Send the penguins to specified display\n"
	    "  -m, --delay          <millisecs>  Set delay between frames\n"));
   printf(_("  -n, --penguins       <n>          Create <n> penguins (max %d)\n"),
	  PENGUIN_MAX);
   printf(_("  -q, --quiet                       Suppress all non-fatal messages\n"
	    "  -v, --version                     Show version information\n"
	    "  -h, --help                        Show this message\n"
	    "  -c, --config-dir     <dir>        Look for config files (and themes) in <dir>\n"
	    "  -p, --ignorepopups                Penguins ignore \"popup\" windows\n"
	    "  -r, --rectwin                     Regard shaped windows as rectangular\n"
	    "  -t, --theme          <theme>      Use named <theme>\n"
	    "  -l, --list-themes                 List available themes\n"
	    "  -i, --theme-info                  Describe a theme and exit (use with -t)\n"
	    "  -b, --no-blood                    Do not show any gory images\n"
	    "  -a, --no-angels                   Do not show any cherubim\n"
	    "  -s, --squish                      kill penguins with mouse\n"
	    "      --debug                        window-map / spawn diagnostics\n"
	    "      --overlay                     force transparent overlay (compositor)\n"
	    "      --no-overlay, --root          force classic root-window drawing\n"
	    "      --all                         Run all available themes simultaneously\n"
	    "      --id             <window id>  Send penguins to window with this ID\n"
	    "      --nice           <ld1> <ld2>  Start killing penguins when load reaches\n"
	    "                                       <ld1>, kill all if load reches <ld2>\n"
	    "(\"--\" can be replaced with \"-\" in all cases)\n"
	    "System data directory: "));
   printf(PKGDATADIR "\n");
}

void
ListThemes()
{
  char **theme;
  int n;
  int nthemes = 0;

  theme = xpenguins_list_themes(&nthemes);
  if (theme == NULL || nthemes == 0) {
    if (!config_dir) {
      config_dir = XPENGUINS_SYSTEM_DIRECTORY;
    }
    else if (xpenguins_verbose) {
      fprintf(stderr, _("No valid themes found (looked in %s%s and %s%s)\n"),
	      config_dir, XPENGUINS_THEME_DIRECTORY, getenv("HOME"),
	      XPENGUINS_USER_DIRECTORY XPENGUINS_THEME_DIRECTORY);
    }
  }
  else {
    for (n = 0; n < nthemes; ++n) {
      fprintf(stdout, "%s\n", theme[n]);
    }
  }
  exit(0);
}

void
DescribeThemes(char **theme_names)
{
  char **info;
  char *location;

  while (*theme_names) {
    location = xpenguins_theme_directory(*theme_names);
    if (!location) {
      fprintf(stderr, _("Theme called \"%s\" not found\n"),
	      xpenguins_remove_underscores(*theme_names));
      exit(2);
    }

    info = xpenguins_theme_info(*theme_names);
    if (!info) {
      /* Assume an informative error message has already been delivered */
      exit(2);
    }
  
    fprintf(stdout, _("Theme: %s\n"), xpenguins_remove_underscores(*theme_names));
    fprintf(stdout, _("Date: %s\n"), info[PENGUIN_DATE]);
    fprintf(stdout, _("Artist(s): %s\n"), info[PENGUIN_ARTIST]);
    fprintf(stdout, _("Copyright: %s\n"), info[PENGUIN_COPYRIGHT]);
    fprintf(stdout, _("License: %s\n"), info[PENGUIN_LICENSE]);
    fprintf(stdout, _("Maintainer: %s\n"), info[PENGUIN_MAINTAINER]);
    fprintf(stdout, _("Location: %s\n"), location);
    fprintf(stdout, _("Icon: %s\n"), info[PENGUIN_ICON]);
    fprintf(stdout, _("Comment: %s\n\n"), info[PENGUIN_COMMENT]);
    ++theme_names;
  }
  exit(0);
}

/* Get the 1-min averaged system load on linux systems - it's the
 * first number in the /proc/loadavg pseudofile. Return -1 if not
 * found. */
float
LoadAverage()
{
  FILE *loadfile = fopen("/proc/loadavg", "r");
  float load = -1;

  if (loadfile) {
    fscanf(loadfile, "%f", &load);
    fclose(loadfile);
  }

  return load;
}

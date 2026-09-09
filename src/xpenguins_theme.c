/* xpenguins_theme.c - provides the themability of xpenguins
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glob.h>

#include "xpenguins.h"
#include <X11/xpm.h>

char *xpenguins_directory = XPENGUINS_SYSTEM_DIRECTORY;
char xpenguins_verbose = 1;

#define MAX_STRING_LENGTH 513

/* Print a warning message */
static
void
__xpenguins_out_of_memory(int line)
{
  fprintf(stderr, _("%s: Out of memory at line %d\n"), __FILE__, line);
}

#define xpenguins_out_of_memory() __xpenguins_out_of_memory(__LINE__)

/* Initialise an XPenguinsTheme structure ready to be filled
 * with data */
static
int
__xpenguins_theme_init(XPenguinsTheme *theme)
{
  int j;
  if (!(theme->data = malloc(PENGUIN_NGENERA * sizeof(ToonData *)))) {
    return 1;
  }
  if (!(theme->data[0] = malloc(PENGUIN_NTYPES * sizeof(ToonData)))) {
    return 1;
  }
  if (!(theme->name = malloc(PENGUIN_NGENERA * sizeof(char *)))) {
    return 1;
  }
  theme->name[0] = NULL;
  if (!(theme->number = malloc(PENGUIN_NGENERA * sizeof(int)))) {
    return 1;
  }
  theme->number[0] = 1;
  theme->ngenera = 1;
  theme->delay = 60; /* milliseconds */
  theme->_nallocated = PENGUIN_NGENERA;

  for (j = 0; j < PENGUIN_NTYPES; ++j) {
    ToonData *data = theme->data[0] + j;
    data->exists = 0;
    data->image = NULL;
    data->filename = NULL;
  }
  return 0;
}

/* Grow a theme ready to accept another genus. */
static
int
__xpenguins_theme_grow(XPenguinsTheme *theme)
{
  int j;
  if (theme->_nallocated == theme->ngenera) {
    if (!(theme->data = realloc(theme->data, (theme->ngenera + PENGUIN_NGENERA)
				* sizeof(ToonData *)))) {
      return 1;
    }
    if (!(theme->name = realloc(theme->name, (theme->ngenera + PENGUIN_NGENERA)
				* sizeof(char *)))) {
      return 1;
    }
    if (!(theme->number = realloc(theme->number, (theme->ngenera + PENGUIN_NGENERA)
				  * sizeof(int)))) {
      return 1;
    }
    theme->_nallocated += PENGUIN_NGENERA;
  }

  if (!(theme->data[theme->ngenera] = malloc(PENGUIN_NTYPES * sizeof(ToonData)))) {
    return 1;
  }
  theme->name[theme->ngenera] = NULL;
  theme->number[theme->ngenera] = 1;

  for (j = 0; j < PENGUIN_NTYPES; ++j) {
    ToonData *data = theme->data[theme->ngenera] + j;
    data->exists = 0;
    data->image = NULL;
    data->filename = NULL;
  }
  ++(theme->ngenera);
  return 0;
}

/* Set the data directory to use to find themes */
void
xpenguins_set_directory(char *directory)
{
  xpenguins_directory = directory;
  return;
}     

/* Remove underscores from a theme name */
char *
xpenguins_remove_underscores(char *name)
{
  char *ch = name;
  while (*ch != '\0') {
    if (*ch == '_') {
      *ch = ' ';
    }
    ++ch;
  }
  return name;
}

/* Return a NULL-terminated list of names of apparently valid themes -
 * basically the directory names from either the user or the system
 * theme directories that contain a file called "config" are
 * returned. Underscores in the directory names are converted into
 * spaces, but directory names that already contain spaces are
 * rejected. This is because the install program seems to choke on
 * directory names containing spaces, but theme names containing
 * underscores are ugly. The list should be freed with
 * xpenguins_free_list(). If nthemes is not NULL then it is used to
 * return the number of themes that were found */
char **
xpenguins_list_themes(int *nthemes)
{
  char *home = getenv("HOME");
  char *home_root = XPENGUINS_USER_DIRECTORY;
  char *themes = XPENGUINS_THEME_DIRECTORY;
  char *config = XPENGUINS_CONFIG;
  char *config_path, *tmp_path;
  char *list = NULL;
  char **list_array = NULL;
  int list_len = 0;
  int i, string_length = 0;
  glob_t globbuf;
  int n = 0;

  /* First add themes from the users home directory */
  string_length = strlen(home) + strlen(home_root)
    + strlen(themes) + 2 + strlen(config) + 1;
  config_path = malloc(string_length);
  if (!config_path) {
    xpenguins_out_of_memory();
    return NULL;
  }
  snprintf(config_path, string_length, "%s%s%s/*%s",
	   home, home_root, themes, config);
  if (glob(config_path, 0, NULL, &globbuf) == GLOB_NOSPACE) {
    free(config_path);
    xpenguins_out_of_memory();
    return NULL;
  }

  /* Then add system themes */
  string_length = strlen(xpenguins_directory) + strlen(themes) + 2
    + strlen(config) + 1;
  tmp_path = realloc(config_path, string_length);
  if (!tmp_path) {
    globfree(&globbuf);
    free(config_path);
    xpenguins_out_of_memory();
    return NULL;
  }
  else {
    config_path = tmp_path;
  }
  snprintf(config_path, string_length, "%s%s/*%s",
	   xpenguins_directory, themes, config);
  if (glob(config_path, GLOB_APPEND, NULL, &globbuf) == GLOB_NOSPACE) {
    free(config_path);
    xpenguins_out_of_memory();
    return NULL;
  }
  free(config_path);

  /* Now add just the sub directory name to a list */
  for (i = 0; i < globbuf.gl_pathc; ++i) {
    char *file_path = globbuf.gl_pathv[i];
    int file_path_len = strlen(file_path);
    char *dir_name = file_path + file_path_len - strlen(config);
    int dir_name_len = 0;
    char valid_name = 1;

    while (dir_name > file_path && dir_name[-1] != '/') {
      --dir_name;
      ++dir_name_len;
      /* We convert all underscores in the directory name 
       * to spaces, but actual spaces in the directory
       * name are not allowed. */
      if (*dir_name == ' ') {
	valid_name = 0;
	break;
      }
      else if (*dir_name == '_') {
	*dir_name = ' ';
      }
    }

    /* Check that theme with this name does not exist - a user may
     * wish to override a system theme, but we don't want duplication
     * in the list that is returned. */
    if (list && valid_name) {
      char *listp = list;
      char *namep = dir_name;
      char possible = 1;
      do {
	if (*listp == '/' || *listp == '\0') {
	  if (possible && *namep == '/') {
	    /* Found a duplicate name */
	    valid_name = 0;
	    break;
	  }
	  else {
	    namep = dir_name;
	    possible = 1;
	  }
	}
	else if (possible) {
	  if (*listp != *namep) {
	    possible = 0;
	  }
	  else {
	    ++namep;
	  }
	}
      }
      while (*(listp++) != '\0');
    }

    if (dir_name_len && valid_name) {
      /* Add theme name to list */
      char *tmp_list;
      if (list) {
	list[list_len-1] = '/';
      }
      list_len += (dir_name_len + 1);
      tmp_list = realloc(list, list_len);
      if (!tmp_list) {
	if (list) {
	  free(list);
	}
	xpenguins_out_of_memory();
	globfree(&globbuf);
	return NULL;
      }
      else {
	list = tmp_list;
      }

      strncpy(list + list_len - dir_name_len - 1, dir_name, dir_name_len);
      list[list_len - 1] = '\0';
      ++n;
    }
  }
  globfree(&globbuf);

  if (list_len) {
    /* Allocate the array of pointers to point to each theme name */
    char *ch = list;
    int j = 1;
    list_array = (char **) malloc((n+1) * sizeof(char *));
    if (!list_array) {
      if (list) {
	free(list);
      }
      xpenguins_out_of_memory();
      return NULL;
    }
    list_array[0] = list;
    while (j < n) {
      if (*ch == '/') {
	*ch = '\0';
	list_array[j++] = ++ch;
      }
      else {
	++ch;
      }
    }
    list_array[j] = NULL;
    if (nthemes) {
      *nthemes = n;
    }
    return list_array;
  }

  return NULL;
}

/* Generic function for freeing character lists that are allocated for
 * verious purposes. */
void
xpenguins_free_list(char **list)
{
  if (list) {
    if (*list) {
      free(*list);
    }
    free(list);
  }
}

/* Return the full path of the specified theme. Spaces in the theme
 * name are converted into underscores. */
char *
xpenguins_theme_directory(char *name)
{
  char *home = getenv("HOME");
  char *home_root = XPENGUINS_USER_DIRECTORY;
  char *themes = XPENGUINS_THEME_DIRECTORY;
  char *config = XPENGUINS_CONFIG;
  char *config_path, *tmp_path, *ch;
  int string_length = 0, root_length;
  struct stat stat_buf;

  /* First look in $HOME/.xpenguins/themes for config */
  root_length = strlen(home) + strlen(home_root) + strlen(themes) + 1;
  string_length = root_length + strlen(name) + strlen(config) + 1;
  config_path = malloc(string_length);
  if (!config_path) {
    xpenguins_out_of_memory();
    return NULL;
  }
  snprintf(config_path, string_length, "%s%s%s/%s%s",
	   home, home_root, themes, name, config);
  /* Convert spaces to underscores */
  ch = config_path + root_length;
  while (*ch != '/') {
    if (*ch == ' ') {
      *ch = '_';
    }
    ++ch;
  }
  /* See if theme exists */
  if (stat(config_path, &stat_buf) == 0) {
    config_path[string_length-strlen(config)-1] = '\0';
    return config_path;
  }
  /* Theme not found in users theme directory... */
  /* Now look in [xpenguins_directory]/themes for config */
  root_length = strlen(xpenguins_directory) + strlen(themes) + 1;
  string_length = root_length + strlen(name) + strlen(config) + 1;
  tmp_path = realloc(config_path, string_length);
  if (!tmp_path) {
    xpenguins_out_of_memory();
    free(config_path);
    return NULL;
  }
  else {
    config_path = tmp_path;
  }
  snprintf(config_path, string_length, "%s%s/%s%s",
	   xpenguins_directory, themes, name, config);
  /* Convert spaces to underscores */
  ch = config_path + root_length;
  while (*ch !='/') {
    if (*ch == ' ') {
      *ch = '_';
    }
    ++ch;
  }
  /* Look for theme */
  if (stat(config_path, &stat_buf) == 0) {
    config_path[string_length-strlen(config)-1] = '\0';
    return config_path;
  }
  /* Theme not found */
  return NULL;
}

/* Copy select properties from one ToonData structure to another */
static
void
__xpenguins_copy_properties(ToonData *src, ToonData *dest)
{
  dest->nframes = src->nframes;
  dest->ndirections = src->ndirections;
  dest->width = src->width;
  dest->height = src->height;
  dest->acceleration = src->acceleration;
  dest->speed = src->speed;
  dest->terminal_velocity = src->terminal_velocity;
  dest->conf = src->conf;
  dest->loop = src->loop;
  dest->master = src->master;
  return;
}

/* Append the theme named "name" to the theme structure "theme". To
 * initialise the theme, set theme->ngenera to 0 before calling this
 * function. On error, a static error message is returned, on success
 * NULL (i.e. 0) is returned. */
static
char *
__xpenguins_append_theme(char *name, XPenguinsTheme *theme)
{
  FILE *config;
  char *file_name = xpenguins_theme_directory(name);
  char *file_base, *xpm_file_name, *tmp_name;
  char word[MAX_STRING_LENGTH];
  static char *out_of_memory;
  ToonData def; /* The default */
  ToonData duf; /* If toon type is unknown... */
  ToonData *current = &def;
  int i, j, word_length, tmp_int;
  int max_file_name_length = 0;
  unsigned int genus = theme->ngenera;
  unsigned int first_genus = theme->ngenera;
  char started = 0;

  out_of_memory = _("Out of memory");

  /* Find theme */
  if (!file_name) {
    return _("Theme not found or config file not present");
  }

  /* Open config file */
  max_file_name_length = strlen(file_name) + 1 + MAX_STRING_LENGTH;
  tmp_name = realloc(file_name, max_file_name_length);
  if (!tmp_name) {
    free(file_name);
    return out_of_memory;
  }
  else {
    file_name = tmp_name;
  }
  file_base = file_name + strlen(file_name);
  *file_base = '/';
  ++file_base;
  snprintf(file_base, MAX_STRING_LENGTH, "config");
  config = fopen(file_name, "r");
  if (!config) {
    free(file_name);
    return _("Unreadable config file");
  }

  /* Allocate space for theme if this is the first one loaded */
  if (genus == 0) {
    if (__xpenguins_theme_init(theme)) {
      return out_of_memory;
    }
  }
  else {
    if (__xpenguins_theme_grow(theme)) {
      xpenguins_free_theme(theme);
      return out_of_memory;
    }
  }

  /* Set default values */
  def.nframes = 1;
  def.ndirections = 1;
  def.width = PENGUIN_DEFAULTWIDTH;
  def.height = PENGUIN_DEFAULTHEIGHT;
  def.acceleration = 0;
  def.speed = 4;
  def.terminal_velocity = 0;
  def.conf = TOON_DEFAULTS;
  def.loop = 0;
  def.master = NULL;

  /* Read config file */
#define NEXT_WORD() (word_length = \
      xpenguins_read_word(config, MAX_STRING_LENGTH, word))
#define WORD_IS(string) (strcasecmp(word, string) == 0)
#define CURRENT_TYPE(type) started = 1; __xpenguins_copy_properties(&def, \
      current = theme->data[genus] + type)
#define GET_INT(dest) if (NEXT_WORD() > 0) { dest = atoi(word); }
#define GET_UINT(dest) \
      if (NEXT_WORD() > 0 && (tmp_int = atoi(word)) >= 0) { dest = tmp_int; }

#define WARNING if(xpenguins_verbose) fprintf

  while(NEXT_WORD() != 0) {
    if (word_length < 0) {
      WARNING(stderr, _("Warning: %s: \"%s...\" ignored: longer than %d characters\n"),
		file_name, word, MAX_STRING_LENGTH);
      continue;
    }
    /* Define a new genus of toon */
    else if (WORD_IS("toon")) {
      if (NEXT_WORD() > 0) {
	if (started) {
	  if (__xpenguins_theme_grow(theme)) {
	    xpenguins_free_theme(theme);
	    return out_of_memory;
	  }
	  ++genus;
	}
	else {
	  started = 1;
	}
	if (!(theme->name[genus] = strdup(word))) {
	  xpenguins_free_theme(theme);
	  return out_of_memory;
	}
      }
      else if (word_length < 0) {
	WARNING(stderr, _("Warning: %s: \"%s...\" ignored: longer than %d characters\n"),
		file_name, word, MAX_STRING_LENGTH);
      }
      else {
	WARNING(stderr, _("Warning: theme config file ended unexpectedly\n"));
      }
    }
    /* Set preferred frame delay in milliseconds */
    else if (WORD_IS("delay")) { GET_UINT(theme->delay); }
    /* Various types of toon */
    else if (WORD_IS("define")) { 
      if (NEXT_WORD() > 0) {
	/* Start defining the default properties of the current genus */
	if (WORD_IS("default")) { current = &def; }
	/* Now the other types of toon */
	else if (WORD_IS("walker")) { CURRENT_TYPE(PENGUIN_WALKER); }
	else if (WORD_IS("faller")) { CURRENT_TYPE(PENGUIN_FALLER); }
	else if (WORD_IS("tumbler")) { CURRENT_TYPE(PENGUIN_TUMBLER); }
	else if (WORD_IS("floater")) { CURRENT_TYPE(PENGUIN_FLOATER); }
	else if (WORD_IS("climber")) { CURRENT_TYPE(PENGUIN_CLIMBER); }
	else if (WORD_IS("runner")) { CURRENT_TYPE(PENGUIN_RUNNER); }
	else if (WORD_IS("action0")) { CURRENT_TYPE(PENGUIN_ACTION0); }
	else if (WORD_IS("action1")) { CURRENT_TYPE(PENGUIN_ACTION1); }
	else if (WORD_IS("action2")) { CURRENT_TYPE(PENGUIN_ACTION2); }
	else if (WORD_IS("action3")) { CURRENT_TYPE(PENGUIN_ACTION3); }
	else if (WORD_IS("action4")) { CURRENT_TYPE(PENGUIN_ACTION4); }
	else if (WORD_IS("action5")) { CURRENT_TYPE(PENGUIN_ACTION5); }
	else if (WORD_IS("exit"))  {
	  CURRENT_TYPE(PENGUIN_EXIT);
	  current->conf |= (TOON_NOCYCLE | TOON_INVULNERABLE);
	}
	else if (WORD_IS("explosion")) {
	  CURRENT_TYPE(PENGUIN_EXPLOSION);
	  current->conf |= (TOON_NOCYCLE | TOON_INVULNERABLE | TOON_NOBLOCK);
	}
	else if (WORD_IS("splatted")) {
	  CURRENT_TYPE(PENGUIN_SPLATTED);
	  current->conf |= (TOON_NOCYCLE | TOON_INVULNERABLE);
	}
	else if (WORD_IS("squashed")) {
	  CURRENT_TYPE(PENGUIN_SQUASHED);
	  current->conf |= (TOON_NOCYCLE | TOON_INVULNERABLE | TOON_NOBLOCK);
	}
	else if (WORD_IS("zapped")) {
	  CURRENT_TYPE(PENGUIN_ZAPPED);
	  current->conf |= (TOON_NOCYCLE | TOON_INVULNERABLE | TOON_NOBLOCK);
	}
	else if (WORD_IS("angel")) {
	  CURRENT_TYPE(PENGUIN_ANGEL);
	  current->conf |= (TOON_INVULNERABLE | TOON_NOBLOCK);
	}
	else /* Unknown type! */ {
	  WARNING(stderr, _("Warning: unknown type \"%s\": ignoring\n"), word);
	  current = &duf;
	}
      }
      else {
	WARNING(stderr, _("Warning: theme config file ended unexpectedly\n"));
      }
    }
    /* Toon properties */
    else if (WORD_IS("width")) { GET_UINT(current->width); }
    else if (WORD_IS("height")) { GET_UINT(current->height); }
    else if (WORD_IS("frames")) { GET_UINT(current->nframes); }
    else if (WORD_IS("directions")) { GET_UINT(current->ndirections); }
    else if (WORD_IS("speed")) { GET_UINT(current->speed); }
    else if (WORD_IS("acceleration")) { GET_UINT(current->acceleration); }
    else if (WORD_IS("terminal_velocity")) { GET_UINT(current->terminal_velocity); }
    else if (WORD_IS("loop")) { GET_INT(current->loop); }
    else if (WORD_IS("pixmap")) { 
      if (NEXT_WORD() > 0) {
	if (current == &def) {
	  WARNING(stderr, _("Warning: theme config file may not specify a default pixmap\n"));
	}
	else if (current == &duf) {
	  /* Don't want to allocate any memory for an unused type */
	  continue;
	}
	else {
	  int status;
	  int igenus, itype; /* For scanning for duplicated pixmaps */
	  char new_pixmap = 1;
	  if (word[0] == '/') {
	    xpm_file_name = word;
	  }
	  else {
	    snprintf(file_base, MAX_STRING_LENGTH, "%s", word);
	    xpm_file_name = file_name;
	  }
	  if (current->image) {
	    /* Pixmap is already defined! */
	    WARNING(stderr, _("Warning: resetting pixmap to %s\n"), word);
	    if (!current->master) {
	      /* Free old pixmap if it is not a copy */
	      XpmFree(current->image);
	      current->image = NULL;
	      if (current->filename) {
		free(current->filename);
	      }
	      current->exists = 0;
	    }
	  }

	  /* Check if the file has been used before, but only look in
             the pixmaps for the current theme... */
	  for (igenus = first_genus; igenus <= genus && new_pixmap; ++igenus) {
	    ToonData *data = theme->data[igenus];
	    for (itype = 0; itype < PENGUIN_NTYPES && new_pixmap; ++itype) {
	      if (data[itype].filename && !data[itype].master
		  && data[itype].exists
		  && strcmp(xpm_file_name, data[itype].filename) == 0) {
		current->master = data + itype;
		current->exists = 1;
		current->filename = data[itype].filename;
		current->image = data[itype].image;
		new_pixmap = 0;
	      }
	    }
	  }

	  if (new_pixmap) {
	    status = XpmReadFileToData(xpm_file_name, &(current->image));
	    switch (status) {
	    case XpmSuccess:
	      current->exists = 1;
	      current->filename = strdup(xpm_file_name);
	      current->master = NULL;
	      break;
	    case XpmNoMemory:
	      fclose(config);
	      free(file_name);
	      xpenguins_free_theme(theme);
	      return out_of_memory;
	      break;
	    case XpmOpenFailed:
	      WARNING(stderr, _("Warning: could not read %s\n"), xpm_file_name);
	      break;
	    case XpmFileInvalid:
	      WARNING(stderr, _("Warning: %s is not a valid xpm file\n"), xpm_file_name);
	      break;
	    }
	  }
	}	      
      }
      else if (word_length < 0) {
	WARNING(stderr, _("Warning: %s: \"%s...\" ignored: longer than %d characters\n"),
		  file_name, word, MAX_STRING_LENGTH);
      }
      else {
	WARNING(stderr, _("Warning: theme config file ended unexpectedly\n"));
      }
    }
    else if (WORD_IS("number")) {
      GET_UINT(theme->number[genus]); 
    }
    else {
      WARNING(stderr, _("Warning: %s ignored in config file\n"), word);
    }
  }
  fclose(config);
  free(file_name);

  theme->ngenera = genus+1;

  /* Now validate our widths, heights etc. with the size of the image */
  for (i = first_genus; i < theme->ngenera; ++i) {
    for (j = 0; j < PENGUIN_NTYPES; ++j) {
      current = theme->data[i] + j;
      if (current->exists) {
	unsigned int imwidth, imheight;
	int values;
	values = sscanf(current->image[0], "%d %d", &imwidth, &imheight);
	if (values < 2) {
	  WARNING(stderr, _("Error reading width and height of %s\n"),
		  current->filename);
	  xpenguins_free_theme(theme);
	  return _("Invalid xpm data");
	}
	if (imwidth < current->width * current->nframes) {
	  if ((current->nframes = imwidth / current->width) < 1) {
	    xpenguins_free_theme(theme);
	    return _("Width of xpm image too small for even a single frame");
	  }
	  else {
	    WARNING(stderr, _("Warning: width of %s is too small to display all the frames\n"),
		    current->filename);
	  }
	}
	if (imheight < current->height * current->ndirections) {
	  if ((current->ndirections = imheight / current->height) < 1) {
	    xpenguins_free_theme(theme);
	    return _("Height of xpm image too small for even a single frame");
	  }
	  else {
	    WARNING(stderr, _("Warning: height of %s is too small to display all the frames\n"),
		    current->filename);
	  }
	}
      }
    }
    if ((!theme->data[i][PENGUIN_WALKER].exists)
	|| (!theme->data[i][PENGUIN_FALLER].exists)) {
      xpenguins_free_theme(theme);
      return _("Theme must contain at least walkers and fallers");
    }
  }

  /* Work out the total number */
  theme->total = 0;
  for (i = first_genus; i < theme->ngenera; ++i) {
    theme->total += theme->number[i];
  }
  if (theme->total > PENGUIN_MAX) {
    theme->total = PENGUIN_MAX;
  }

  penguin_data = theme->data;
  penguin_ngenera = theme->ngenera;
  penguin_numbers = theme->number;
  return NULL;
}

/* Load the named theme */
char *
xpenguins_load_theme(char *name, XPenguinsTheme *theme)
{
  theme->ngenera = 0;
  return __xpenguins_append_theme(name, theme);
}

/* Load the themes in the string array (where the array is NULL
 * terminated, as well as all the strings that are pointed to) */
char *
xpenguins_load_themes(char **names, XPenguinsTheme *theme)
{
  theme->ngenera = 0;
  while (*names) {
    char *error;
    error = __xpenguins_append_theme(*names, theme);
    if (error) {
      return error;
    }
    ++names;
  }
  return NULL;
}

/* Free all the data associated with a theme */
void
xpenguins_free_theme(XPenguinsTheme *theme)
{
  int i, j;
  for (i = 0; i < theme->ngenera; ++i) {
    for (j = 0; j < PENGUIN_NTYPES; ++j) {
      ToonData *data = theme->data[i] + j;
      if (data->exists && data->image && !data->master) {
	XpmFree(data->image);
	data->exists = 0;
	data->image = NULL;
	if (data->filename) {
	  free(data->filename);
	  data->filename = NULL;
	}
      }
    }
    if (theme->name[i]) {
      free(theme->name[i]);
      theme->name[i] = NULL;
    }
    free(theme->data[i]);
  }
  free(theme->data);
  free(theme->name);
  free(theme->number);
  theme->_nallocated = 0;
  theme->ngenera = 0;
  return;
}

/* Print basic theme information to standard error - for debugging
 * purposes */
void
xpenguins_describe_theme(XPenguinsTheme *theme)
{
  int i, j;
  if (!theme) {
    fprintf(stderr, _("No theme!\n"));
    return;
  }
  for (i = 0; i < theme->ngenera; ++i) {
    if (theme->name[i]) {
      fprintf(stdout, "%s\n", theme->name[i]);
    }
    for (j = 0; j < PENGUIN_NTYPES; ++j) {
      ToonData *data = theme->data[i] + j;
      if (data->exists) {
	fprintf(stderr, _("  Toon %d: %s %ux%u %ux%u\n"), i, data->filename,
		data->nframes, data->ndirections, data->width, data->height);
      }
    }
  }
}

#define ADD_STRING(index) \
  if (string_length) { \
    if (!(tmp_string = realloc(list, list_length + string_length + 1))) { \
       xpenguins_out_of_memory(); \
       free(file_name); free(theme_dir); free(list); \
       return NULL; \
    } \
    else { list = tmp_string; \
       offsets[index] = list_length; \
       strncpy(list + offsets[index], string, string_length + 1); \
       list_length += (string_length + 1); \
    }; \
  }

/* Read the "about" file in a theme directory and return a string list
 * of theme properties. This list should be freed with
 * xpenguins_free_list(). */
char **
xpenguins_theme_info(char *name)
{
  char word[MAX_STRING_LENGTH];
  char string[MAX_STRING_LENGTH];
  char **info;
  char *list;
  char *theme_dir = xpenguins_theme_directory(name);
  char *file_name;
  char *about = "/about";
  FILE *about_file;

  int i, file_name_length;
  int word_length, string_length;
  int about_length = strlen(about);
  int theme_dir_length;
  int list_length = 0;
  int offsets[PENGUIN_NABOUTFIELDS];

  /* Determine "about" filename string */
  if (!theme_dir) {
    return NULL;
  }
  theme_dir_length = strlen(theme_dir);
  file_name_length = theme_dir_length + about_length + 1;
  file_name = malloc(file_name_length);
  if (!file_name) {
    xpenguins_out_of_memory();
    free(file_name);
    free(theme_dir);
    return NULL;
  }
  snprintf(file_name, file_name_length, "%s%s", theme_dir, about);

  for (i = 0; i < PENGUIN_NABOUTFIELDS; ++i) {
    offsets[i] = 0;
  }

  /* Open "about" file */
  about_file = fopen(file_name, "r");
  if (!about_file) {
    fprintf(stderr, _("Could not read %s\n"), file_name);
    free(file_name);
    free(theme_dir);
    return NULL;
  }

  list = malloc(1);
  if (!list) {
    xpenguins_out_of_memory();
    free(file_name);
    free(theme_dir);
    return NULL;
  }
  list_length = 1;
  *list = '\0';

  while ((word_length = xpenguins_read_word(about_file,
					    MAX_STRING_LENGTH,
					    word))) {
    char *tmp_string;
    string_length = xpenguins_read_line(about_file,
					MAX_STRING_LENGTH,
					string);

    if (WORD_IS("artist") && !offsets[PENGUIN_ARTIST]) {
      ADD_STRING(PENGUIN_ARTIST);
    }
    else if (WORD_IS("maintainer") && !offsets[PENGUIN_MAINTAINER]) {
      ADD_STRING(PENGUIN_MAINTAINER);
    }
    else if (WORD_IS("date") && !offsets[PENGUIN_DATE]) {
      ADD_STRING(PENGUIN_DATE);
    }
    else if (WORD_IS("copyright") && !offsets[PENGUIN_COPYRIGHT]) {
      ADD_STRING(PENGUIN_COPYRIGHT);
    }
    else if (WORD_IS("license") && !offsets[PENGUIN_LICENSE]) {
      ADD_STRING(PENGUIN_LICENSE);
    }
    else if (WORD_IS("comment") && !offsets[PENGUIN_COMMENT]) {
      ADD_STRING(PENGUIN_COMMENT);
    }
    else if (WORD_IS("icon") && !offsets[PENGUIN_ICON]) {
      if (string_length && string_length < MAX_STRING_LENGTH-1) {
	if (string[0] == '/') {
	  ADD_STRING(PENGUIN_ICON);
	}
	else if (!(tmp_string = realloc(list, list_length + theme_dir_length 
					+ string_length + 2 ))) {
	  xpenguins_out_of_memory();
	  free(file_name);
	  free(theme_dir);
	  free(list);
	  return NULL;
	} else {
	  list = tmp_string;
	  offsets[PENGUIN_ICON] = list_length;
	  snprintf(list + offsets[PENGUIN_ICON], theme_dir_length + string_length + 2,
		   "%s/%s", theme_dir, string);
	  list_length += (theme_dir_length + string_length + 2);
	}
      }
    }
  }

  /* Allocate array of strings */
  info = malloc(PENGUIN_NABOUTFIELDS * sizeof(char *));
  if (!info) {
    xpenguins_out_of_memory();
    free(file_name);
    free(theme_dir);
    return NULL;
  }

  info[0] = list;
  for (i = 1; i < PENGUIN_NABOUTFIELDS; ++i) {
    info[i] = list + offsets[i];
  }
  free(file_name);
  free(theme_dir);
  return info;
}

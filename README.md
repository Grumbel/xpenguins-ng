# xpenguins-ng

Cool little penguins walking along the tops of your windows.

**xpenguins-ng** is a maintained fork of [XPenguins](http://xpenguins.seul.org/)
2.2 by Robin Hogan. Version **3.0** keeps the classic behaviour and theme
format while drawing through a shaped overlay so the toons stay visible on
modern X11 desktops (with or without a compositor).

Copyright (C) 1999–2001 Robin Hogan  
Copyright (C) 2026 Ingo Ruhnke \<grumbel@gmail.com\>

License: GNU GPL v2 or later (see `COPYING` / `LICENSES/GPL-2.0-or-later.txt`).

## Introduction

xpenguins-ng animates cartoons on your X11 desktop. By default these are
penguins: they fall from the top of the screen, walk along the tops of
windows, climb the sides, and do various other things. Themes in the base
package are **Penguins**, **Big Penguins** (50% larger), **Classic Penguins**
(XPenguins 1.2 style), **Turtles**, and **Bill**. More themes were historically
available from the XPenguins site as the separate `xpenguins_themes` package
(Simpsons, Sonic, Lemmings, Winnie the Pooh, and others).

```bash
xpenguins-ng
xpenguins-ng --help
xpenguins-ng --theme Big_Penguins
xpenguins-ng --verbose
man xpenguins-ng
```

A **system tray** icon (bomber frame) is shown when a tray is available;
click it to exit with the usual death animation. Use `--no-tray` to disable it.

To make your own theme, read the **THEMES** section of the man page and look
at the default theme config (usually
`/usr/share/xpenguins-ng/themes/Penguins/config`, or under your install
prefix). The GIMP scripts `lay-out-frames.scm` and `resize-frames.scm` can
help: the first lays out animated-GIF frames side by side for use as an
xpenguins image; the second resizes frames to reduce colour bleed between
adjacent frames.

### Drawing modes

| Option | Behaviour |
|--------|-----------|
| *(default)* | **Shaped overlay** — full-screen override-redirect window; only toon pixels are visible via XShape (no compositor required) |
| `--overlay` | Same as default (explicit) |
| `--no-overlay` / `--root` | Classic drawing on the desktop / root window |
| `--id 0x…` | Draw on a specific window (classic path) |

XShape is an ordinary X11 extension: the overlay path does **not** depend on
a compositing manager. Classic mode is mainly for debugging or unusual
setups; it erases via `_XROOTPMAP_ID` when the desktop publishes a wallpaper
pixmap.

### Useful options

| Option | Behaviour |
|--------|-----------|
| `--verbose` | Diagnostic messages (tray docking, drawing mode, …) |
| `-q` / `--quiet` | Suppress non-fatal messages (default is already quiet) |
| `--debug` | Window-map / spawn diagnostics |
| `--no-tray` | Do not show a system tray icon |
| `-t` / `--theme` | Select theme by name |
| `-n` / `--penguins` | Number of toons |
| `-s` / `--squish` | Click toons to squish them |

### Theme search path

- System: `share/xpenguins-ng/themes/` (under the install prefix)
- User: `$XDG_DATA_HOME/xpenguins-ng/themes` (default `~/.local/share/xpenguins-ng/themes`)
- Legacy still searched: `~/.xpenguins-ng/themes`, `~/.xpenguins/themes`

## Installation

This project uses **CMake** (Autotools are gone).

Dependencies: X11 (`libX11`, `libXext`, `libXpm`), optional `libXfixes`,
CMake ≥ 3.16, pkg-config.

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
sudo cmake --install build
```

Or with Nix:

```bash
nix build
nix run . -- --theme Penguins
```

See `INSTALL` for more detail.

## Troubleshooting

**Toons invisible or leaving trails**

- Prefer the default overlay path (do not pass `--root` unless you need it).
- Classic mode needs a wallpaper pixmap (`_XROOTPMAP_ID` / `ESETROOT_PMAP_ID`);
  without that, erase falls back to `XClearArea` and may trail.

**Tray icon missing or odd**

- Ensure a status tray / notification area plugin is on the panel (XFCE, etc.).
- Run with `--verbose` to see docking / XEmbed messages.
- Click the icon for a graceful exit (same death animation as Ctrl-C).

**Window tops ignored / wrong**

- Use `--debug` to inspect the solid-window map.
- Some desktop wallpaper windows are excluded on purpose so toons do not walk
  on them.

Many window managers used to put a large “desktop” window over the root;
classic XPenguins tried to find that window and draw on it. That approach is
fragile under modern desktops, which is why the shaped overlay is the default.

## Layout of the source

- **`src/main.c`**, **`xpenguins_*.c`** — CLI, themes, main loop, system tray
- **`src/toon_*.c`** — animation and X drawing (`toon.h`); overlay vs classic
  is selected in `ToonSetupDrawWindow()`
- **`src/toon_root.c`** — still locates a desktop window for geometry / classic
- **`themes/`** — bundled themes and XPM frames
- **`data/`** — desktop file and hicolor icons
- **`flake.nix`** — Nix build from in-tree CMake sources

## Credits

Original XPenguins: Robin Hogan.  
xpenguins-ng maintenance and modern X11 work: Ingo Ruhnke.  
Inspiration: Rick Jansen \<rick@sara.nl\> and the classic **xsnow**.

See `AUTHORS` and the SPDX headers in individual files.

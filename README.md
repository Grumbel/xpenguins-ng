# xpenguins-ng

Cool little penguins walking along the tops of your windows.

**xpenguins-ng** is a maintained fork of [XPenguins](http://xpenguins.seul.org/)
2.2 by Robin Hogan. Version **3.0** targets modern X11 compositors while
keeping the classic behaviour and theme format.

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

On a **compositing** window manager the default drawing path is a **shaped
overlay** so sprites stay visible without painting the root window. Use
`--no-overlay` / `--root` for the classic desktop/root drawing path.

```bash
xpenguins-ng
xpenguins-ng --help
xpenguins-ng --theme Big_Penguins
xpenguins-ng --debug
man xpenguins-ng
```

To make your own theme, read the **THEMES** section of the man page and look
at the default theme config (usually
`/usr/share/xpenguins-ng/themes/Penguins/config`, or under your install
prefix). The GIMP scripts `lay-out-frames.scm` and `resize-frames.scm` can
help: the first lays out animated-GIF frames side by side for use as an
xpenguins image; the second resizes frames to reduce colour bleed between
adjacent frames.

### Overlay vs classic drawing

| Option | Behaviour |
|--------|-----------|
| *(default)* | Shaped overlay when a compositor is detected, else classic |
| `--overlay` | Always use the transparent shaped overlay |
| `--no-overlay` / `--root` | Classic drawing on the desktop/root window |
| `--id 0x…` | Draw on a specific window (classic path) |
| `--debug` | Window-map / spawn diagnostics on stderr |

System themes: `share/xpenguins-ng/themes/`  
User themes: `$XDG_DATA_HOME/xpenguins-ng/themes` (default `~/.local/share/xpenguins-ng/themes`; legacy `~/.xpenguins(-ng)/themes` still searched)

## Installation

This project uses **CMake** (Autotools are gone).

Dependencies: X11 (`libX11`, `libXext`, `libXpm`), optional `libXfixes`,
CMake ≥ 3.16, pkg-config.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
sudo cmake --install build
```

Or with Nix:

```bash
nix build
nix run
```

See `INSTALL` for a short summary.

## License

Released under the GNU General Public License — see `COPYING`. That license
applies to the program and to the base themes **Penguins**, **Classic
Penguins**, **Big Penguins**, **Turtles**, and **Bill**. Per-file copyright
for theme art is recorded in `REUSE.toml` from each theme’s `about` file
(the `about` files remain the primary source).

## Frequently asked questions

**The program is running but I can’t see any penguins. Why?**

Historically, KDE, CDE, Enlightenment, Nautilus, and others placed a large
window over the root window; XPenguins tried to find that window and draw
to it. On **modern compositors** the default is the shaped overlay path.
If you still see nothing:

- Try `xpenguins-ng --overlay` or `xpenguins-ng --no-overlay`
- Run `xpenguins-ng --debug` and check solid candidates / skipped windows
- Maximized windows at `y = 0` are not used as walkable tops (sprites would
  sit fully off-screen); toons land on panels and non-maximized window tops

**How do I select themes with a dialog?**

You can still use something like `Xdialog` (or `zenity`) with a shell
function. Example for bash (`~/.bashrc`):

```bash
function xpselect() {
  themes="$(xpenguins-ng -l | tr '\n' ' ')"
  xpenguins-ng -a -b "$(Xdialog --stdout --menubox \
    "Please select your XPenguins theme..." 20 60 10 $themes \
    Default xpenguins-ng -b "Ok")"
}
```

Adjust for your dialog tool and shell.

## Code guide

You may modify or borrow this code under the GNU GPL. Sources live in `src/`:

- **`toon_*.c`** — animation on the root or overlay window; X details are
  hidden from higher layers. Functions are prefixed with `Toon` (see
  `toon.h`). `ToonGetRootWindow()` in `toon_root.c` still locates an
  appropriate desktop window for the classic path under various WMs.
- **`xpenguins_*.c`** — theme loading and penguin behaviour (`xpenguins.h`).
- **`main.c`** — command-line front end.

The old GNOME applet used the same toon/xpenguins libraries. Prefer changes
that keep that separation intact.

## Acknowledgements

Inspiration: Rick Jansen \<rick@sara.nl\> and the classic **xsnow**.

Many penguin images came from **Pingus** (http://pingus.seul.org/), by
Joel Fauche \<joel.fauche@wanadoo.fr\> and Craig Timpany
\<timpany@es.co.nz\>. Rob Gietema \<tycoon@planetdescent.com\> contributed
images used in the default theme.

## Authors

- Robin Hogan \<R.J.Hogan@reading.ac.uk\> — original XPenguins  
- Ingo Ruhnke \<grumbel@gmail.com\> — xpenguins-ng 3.0  

Original homepage: http://xpenguins.seul.org/  
Related project: https://www.ratrabbit.nl/ratrabbit/software/xpenguins

## See also

Michael Vines rewrote an older version for Windows as **WinPenguins**
(historical link from the 2.2 README:
http://neomueller.org/~isamu/winpenguins/).

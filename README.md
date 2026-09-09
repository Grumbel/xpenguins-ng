# xpenguins-ng

**xpenguins-ng** 3.0 is a maintained fork of [xpenguins](http://xpenguins.seul.org/)
2.2 by Robin Hogan. Cute penguins (and other themes) walk along the tops of
your X11 windows.

This release targets **modern X11 compositors**: a shaped overlay window
keeps sprites visible without trashing the root window, with the classic
root-drawing path still available.

License: GNU GPL v2 or later (same as upstream 2.2).

## Build

Dependencies: X11, libXext, libXpm, libXfixes (optional), CMake ≥ 3.16, pkg-config.

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

## Run

```bash
xpenguins-ng
xpenguins-ng --theme Big_Penguins
xpenguins-ng --help
xpenguins-ng --debug
```

### Overlay vs classic drawing

| Option | Behaviour |
|--------|-----------|
| *(default)* | Use shaped overlay when a compositor is detected, else classic |
| `--overlay` | Always use the transparent shaped overlay |
| `--no-overlay` / `--root` | Always draw on the desktop/root window |
| `--id 0x…` | Draw on a specific window (classic path) |

### Themes

System themes install under `share/xpenguins-ng/themes/`.  
User themes: `~/.xpenguins-ng/themes/`.

## History

- **2.2** (2001) — last upstream release by Robin Hogan  
- **3.0** — xpenguins-ng: CMake, compositor overlay, multi-monitor and
  window-map fixes (see ChangeLog)

Original project: http://xpenguins.seul.org/  
Related modern work: https://www.ratrabbit.nl/ratrabbit/software/xpenguins

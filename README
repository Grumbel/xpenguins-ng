# xpenguins

Cute little penguins (and other themes) that walk along the tops of
your windows. Originally written by Robin Hogan (1999–2001), version
2.2, licensed under the GNU GPL v2 or later.

This tree is a maintained source package of xpenguins 2.2: CMake
build, and a dual drawing path so the toons work on modern X11
compositors as well as classic root-window setups.

## Build

Dependencies: X11 (`libX11`, `libXext`, `libXpm`), optional `libXfixes`
(for click-through input shape on the overlay).

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
sudo cmake --install build
```

Themes install under `${prefix}/share/xpenguins/themes/`.

## Running

```bash
xpenguins
xpenguins --theme Big_Penguins
xpenguins --help
```

### Drawing modes

| Mode | When | Behaviour |
|------|------|-----------|
| **Overlay** (default with compositor) | `_NET_WM_CM_Sn` owned, or `--overlay` | Full-screen ARGB override-redirect window, transparent background, click-through. No trails on the root. |
| **Classic** | No compositor, or `--no-overlay` / `--root`, or `--id` | Draw on the desktop / virtual root found by `ToonGetRootWindow()`. |

Force a mode:

```bash
xpenguins --overlay      # always use transparent overlay
xpenguins --no-overlay   # always draw on the desktop window
xpenguins --root         # same as --no-overlay
xpenguins --id 0x1234    # draw on a specific window (classic path)
```

`--squish` (mouse-kill) disables click-through on the overlay so button
events can be received.

## Themes

System themes: `$prefix/share/xpenguins/themes/`  
User themes: `~/.xpenguins/themes/`

Bundled: Penguins, Big_Penguins, Classic_Penguins, Turtles, Bill.

## History / upstream

- Original project: http://xpenguins.seul.org/
- A separate modern rewrite (GTK3 UI, different codebase) lives at
  https://www.ratrabbit.nl/ratrabbit/software/xpenguins

See `TODO.md` for port status and remaining work. See `ChangeLog` and
`COPYING` for upstream history and license text.

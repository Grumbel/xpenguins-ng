# xpenguins-ng TODO

Project: xpenguins-ng (fork of xpenguins 2.2) as a proper source tree (no longer a
Nix-flake-only wrapper), port to modern X11 compositors, and switch
the build system to CMake.

Original upstream: Robin Hogan, xpenguins-2.2 (2001), GPL-2.0-or-later.
Previous packaging: grumnix/xpenguins Nix flake that fetched the
tarball and applied four small patches.

## Goals

1. Full source directory in git (tarball contents + applied patches).
2. CMake build system (replace autotools).
3. Work correctly under modern compositing WMs (picom, Mutter, KWin,
   xfwm4 compositor, etc.): no drawing on the real root that leaves
   trails or is invisible.
4. Prefer retaining a classic “draw on root / virtual root” path and
   make the compositor-friendly path optional or auto-detected.
5. Fix any other build/runtime issues encountered (includes, prototypes,
   install layout, theme path, etc.).

## Status

### Done

- [x] Extract xpenguins-2.2 tarball into a clean git tree
- [x] Apply historical flake patches:
  - `__ToonErrorHandler` prototype (XSetErrorHandler)
  - `#include <stdlib.h>` for `exit()` in `toon_signal.c`
  - `snprintf(file_base, …, "%s", word)` format fix
- [x] Drop autotools-generated files from the tree
- [x] Initial `.gitignore`
- [x] CMakeLists.txt (binary, man page, themes, `PKGDATADIR`, `config.h`)
- [x] Compositor-friendly transparent overlay path (`ToonSetupDrawWindow`)
  - Auto when `_NET_WM_CM_Sn` is owned; force with `--overlay` /
    `--no-overlay` / `--root`
  - ARGB visual, background pixel 0, click-through via XFixes/XShape
  - Draw/erase use `toon_draw_window`; desktop geometry still from
    `toon_root` / `toon_parent`
  - Overlay excluded from window collision map
  - Classic path kept for non-compositing WMs and `--id`

### Done (continued)

- [x] Drop unused `X11/Intrinsic.h` (no Xt link dependency)
- [x] `ToonSyncDisplaySize()`: resize overlay/squish window when desktop size changes
- [x] Select `StructureNotify` on X root so size changes are noticed
- [x] CMake install verified (`bin/`, `man1/`, `share/xpenguins/themes/*`)
- [x] README updated for CMake + overlay/classic drawing modes

### Done (continued)

- [x] Nix flake builds from in-tree sources via CMake (no tarball / patches)
- [x] Remove obsolete fix-*.diff (fixes are in the source tree)
- [x] SPDX identifiers on new build files (CMakeLists.txt, cmake/config.h.in)

### Done (continued)

- [x] Fix BadMatch on XCopyArea in overlay mode (Xpm pixmaps must use
      overlay visual/colormap/depth)
- [x] Force opaque alpha on ARGB Xpm pixmaps so compositors show toons
- [x] Avoid _NET_WM_WINDOW_TYPE_DESKTOP (can hide under wallpaper)
- [x] Switch overlay to default-depth + XShape (ARGB was invisible)

### Done (continued)

- [x] Remove Autotools files (Makefile.am, configure.in, spec.in)
- [x] INSTALL documents CMake only

### Done (continued)

- [x] Rename README to README.md
- [x] Wire -r/--rectwin to TOON_NOSHAPEDWINDOWS (was a no-op)
- [x] Fix Window sscanf format; drop Xos.h; include time.h
- [x] Document --overlay in man page

### Done (continued)

- [x] Occlusion-aware window map: only visible (uncovered) window
      surface is solid; no walking on tops hidden behind others

### Done (continued)

- [x] Do not squash fallers/tumblers/floaters on TOON_HERE (spawn vs
      maximized windows at y=0 caused instant explosions)

### Done (continued)

- [x] Exclude _NET_WM_WINDOW_TYPE_DESKTOP / wallpaper from solid map
- [x] Add --debug for window-map diagnostics

### Done (continued)

- [x] Rename project to xpenguins-ng, version 3.0

### In progress / next

- [ ] Runtime testing under real compositors (picom, Mutter, KWin)
- [ ] Multi-monitor / Xinerama / per-output overlays (beyond single screen size)
- [ ] Optional: output shape matching toon pixels (less overdraw)
- [ ] Optional: XComposite / XDamage based invalidation if needed
- [ ] Full REUSE.toml / LICENSES layout (optional packaging polish)

### Design notes (compositor path)

Classic code drew with `XCopyArea` / `XClearArea` on `toon_root`. Under
a compositor the root is redirected or covered; clearing leaves trails
and drawing is often invisible.

Current dual-path:

1. Detect compositor via `_NET_WM_CM_Sn` selection owner (or CLI force).
2. Overlay: full-screen override-redirect ARGB window, empty input shape
   (unless `--squish`), `_NET_WM_WINDOW_TYPE_DESKTOP`, lowered.
3. `toon_draw_window` is the overlay; erase clears to transparent bg.
4. `ToonGetRootWindow()` still finds desktop geometry / client parent.
5. Window map for walking still scans children of `toon_parent`.

### Known residual risks

- Overlay stacking vs desktop wallpaper clients may need WM-specific
  tweaks.
- Squish + overlay: click-through disabled; events on overlay window.
- No automated display test in this environment (no real X session).
- Theme path still depends on install-time `PKGDATADIR`.

## Notes for continuation

- Work tree bootstrapped from `http://xpenguins.seul.org/xpenguins-2.2.tar.gz`
  plus the four `fix-*.diff` files from the Nix flake repo.
- Author for commits: Ingo Ruhnke <grumbel@gmail.com>
  with `Co-authored-by: Grok <grok@x.ai>`.
- Deliverables are git bundles (`xpenguins-00N-…`, stacked, HEAD ref).
  History was rebased onto GitHub 66fd29f; use bundles 004+ on that base.
  Bundles 001-003 are obsolete (unrelated root).
- Do not remove features without discussion; prefer dual-path
  (classic root + overlay) over deleting the old drawing code.
- Apply: checkout 66fd29f, then git pull 004, 005, 006, ...

## References

- Original homepage: http://xpenguins.seul.org/
- Modern rewrite (GTK3 overlay, different codebase):
  https://www.ratrabbit.nl/ratrabbit/software/xpenguins
- xsnow / compositor-friendly approaches: transparent fullscreen
  override-redirect window with empty input shape

### Done (continued)

- [x] System tray icon visibility under XFCE
  - Honour `_NET_SYSTEM_TRAY_VISUAL` (ARGB panels) when creating the
    tray window and its colormap
  - Rebuild the opaque bomber icon at the size the panel configures
    (typically 22×22) instead of a fixed 32×32 pixmap
  - Keep BackgroundPixmap + explicit paint on map/reparent/configure/
    embed so Expose-sparse trays still show the icon
  - Log manager visual when verbose

## Notes (tray)

Log sequence that previously produced an invisible slot:

  docked → configure 32x32 → reparent → map → unmap → reparent →
  XEMBED_EMBEDDED_NOTIFY → configure 22x22 → mapped

Docking and XEmbed worked; the icon was blank because the window was
on the default visual while the panel expected its advertised visual,
and the 32×32 BackgroundPixmap did not match the 22×22 slot.

### Done (continued)

- [x] Tray icon still invisible on XFCE ARGB visual (depth 32)
  - XAllocNamedColor / Xpm left alpha=0; panel composites icon away
  - Build TrueColor pixels with full opacity via channel masks
  - XGetImage/XPutImage pass to force alpha on strip + icon pixmap
  - Opaque background_pixel / paint fill for depth-32 windows

### Done (continued)

- [x] Tray still blank with ARGB manager visual + forced alpha
  - Prefer default (opaque) screen visual instead of ARGB without XRender
  - Manager must match icon visual per system-tray spec; default works
  - Defer XMapWindow until XEMBED_EMBEDDED_NOTIFY
  - Only set BackgroundPixmap when pixmap size matches window size

### Done (continued)

- [x] Tray icon visible but cropped (32 into 24) with black background
  - Rebuild icon pixmap to exact ConfigureNotify width×height
  - ParentRelative background so panel colour shows through
  - XShape mask from bomber XPM so only the penguin is opaque
  - Click-to-exit already correct (graceful shutdown)

### Done (continued)

- [x] Tray click exits before death animation finishes
  - toon_exit_requested stayed set; main loop treated every frame as
    another interrupt and broke on interupts > 1
  - Clear toon_exit_requested after consuming it (like ToonSignal)

### Done (continued)

- [x] Tray icon still cropped / black background on XFCE
  - Nearest-neighbour scale of 32×32 bomber frame into the real slot
    (e.g. 22×22) instead of clipping the top-left
  - ParentRelative clear + GC clip mask so only penguin pixels are
    blitted (black underlay never reaches the window)
  - XShape still applied when the extension is present

### Done (continued)

- [x] Tray colours inverted / black background after scale
  - Fix XImage byte/bit order when scaling
  - Auto-detect 1-bit mask polarity (centre pixel of frame must be opaque)
  - Invert mask if needed so ClipMask + XShape draw the penguin, not the hole

### Done (continued)

- [x] Overlapping toons lose colour in transparent regions of the other
  - Overlay path used full-rectangle XCopyArea without clip mask
  - Transparent XPM pixels (often black) overwrote the toon underneath
  - Always apply data->mask as GC clip when blitting (classic + overlay)

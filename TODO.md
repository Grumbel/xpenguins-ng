# xpenguins TODO

Project: revive xpenguins 2.2 as a proper source tree (no longer a
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

### In progress / next

- [x] Add CMakeLists.txt (executable + theme install + man page)
- [x] Define `PKGDATADIR` / theme search path via CMake
- [x] Provide a minimal `config.h` (or generate it) so `HAVE_CONFIG_H`
      and `VERSION` still work

### Compositor / drawing port (main technical work)

Background: the classic code draws with `XCopyArea` / `XClearArea` on
`toon_root` (found by `ToonGetRootWindow()`). Under a compositor the
root is redirected or covered; clearing leaves trails, and drawing is
often invisible.

Preferred approach (keep classic path when possible):

1. Detect compositor / whether root drawing is usable
   (e.g. `_NET_WM_CM_S0` selection owner, or a simple probe).
2. When a compositor is present (or forced), create a full-screen
   override-redirect window with:
   - ARGB visual + 32-bit depth when available
   - input shape empty (click-through) via XFixes or XShape
   - output shape / alpha so only the toon pixels are opaque
   - rest of the window fully transparent
3. Draw toons into that window instead of the root; erase by clearing
   the previous rectangle to transparent (or double-buffer / reshape).
4. Keep `ToonGetRootWindow()` and the old root-drawing path for
   non-compositing WMs and for `--id` / override cases.
5. Window geometry map for walking still comes from scanning client
   windows under `toon_parent` (existing logic in `toon_query.c` /
   associate); that part should remain valid.

Optional later:

- [ ] XComposite / XDamage based invalidation if needed
- [ ] Multi-monitor / RandR awareness beyond current screen size
- [ ] Drop Xt/Intrinsic dependency if it is only used lightly

### Build / packaging polish

- [ ] CMake install: binary, man page, themes under
      `${CMAKE_INSTALL_DATADIR}/xpenguins/themes/...`
- [ ] Nix flake updated to build from the in-tree sources (optional)
- [ ] REUSE / SPDX headers on new files; keep original GPL-2 headers
- [ ] Basic README update describing CMake and compositor behaviour

### Known issues from original / packaging

- Penguins on root leave trails under compositors
- Drawing on root is invisible on many modern DEs
- Autotools `configure` had K&R `main()` (fixed only in generated
  configure in the flake; irrelevant once on CMake)
- Theme path relies on `PKGDATADIR` compile-time define

## Notes for continuation

- Work tree was bootstrapped from `http://xpenguins.seul.org/xpenguins-2.2.tar.gz`
  plus the four `fix-*.diff` files from the Nix flake repo.
- Author for commits: Ingo Ruhnke <grumbel@gmail.com>
  with `Co-authored-by: Grok <grok@x.ai>`.
- Deliverables are git bundles (`xpenguins-001-…`, stacked, HEAD ref).
- Do not remove features without discussion; prefer dual-path
  (classic root + overlay) over deleting the old drawing code.

## References

- Original homepage: http://xpenguins.seul.org/
- Modern rewrite (GTK3 overlay, different codebase): 
  https://www.ratrabbit.nl/ratrabbit/software/xpenguins
  (useful as design reference for transparent click-through windows,
  not as a drop-in replacement of this tree)
- xsnow / compositor-friendly approaches: transparent fullscreen
  override-redirect window with empty input shape

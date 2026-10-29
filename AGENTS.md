# Duke3D on idTech 2 — agent guide

Duke Nukem 3D fully ported to the idTech 2 (Quake 2) engine. The renderer
does not load BSP files: Build `.map` files are loaded natively through
`qcommon/dnmap.cpp`.

This file is mirrored (same content) as `CLAUDE.md` and
`.github/copilot-instructions.md`. When the structure changes, update all
three.

## Repository layout

- `code/` — all engine code that builds. One module per directory:
  - `client/` — client: `cl_main`, entity parsing (`cl_ents`), prediction
    (`cl_pred`), temporary entities (`cl_tent`), view, HUD (`dnhud`), menus
    (`menu.cpp`, `qmenu.cpp`), console, keys, cinematics, save menu.
  - `server/` — `sv_*`: world (`sv_world`), entity snapshotting (`sv_ents`),
    commands (`sv_ccmds`), user commands (`sv_user`), game API glue
    (`sv_game`).
  - `qcommon/` — shared engine code: `cmd`, `cvar`, `files`, `cmodel`
    (collision), `pmove`, `net_chan`, `md4`, `crc`, plus the Duke-native
    heart of the port: `dnmap` (Build `.map` loader), `dnvisibility`,
    `dntransport` (subst/travel moves), `dncon` (CON scripting),
    `dnplayermove`, `dnmath`, `dnpcx`, `dnanimation`, `dnzrange`.
  - `ref_gl/` — the OpenGL renderer module: `gl_image`, `gl_model`,
    `gl_rsurf`, `gl_rmain`, `gl_light`, `gl_mesh`, `gl_warp`, `gl_draw`, the GLSL shader
    backend (`gl_shaders`), the SDL GL context and Doom 3 BFG display/fullscreen
    handling (`glimp_sdl`), and the
    Duke-specific layer `dnrender.cpp`.
  - `sound/` — OpenAL backend (`sndopenal`), music (`sndmusic`, decoded by
    `third-party/mp3` via dr_mp3), ambience.
  - `game/` — the `dngamex86` game module: Duke gameplay (actors, AI
    (`dnnativeai`), combat, sectors, `dnuse`, CON runtime (`dnconrun`),
    save/load, drops, gibs, effects).
  - `cgame/` — the `cgamex86` client game module (weapon view and weapon
    animation).
  - `sdl/` — the SDL3 platform layer: entry point, message pump and events
    (`sdl_sys`), input and joystick (`sdl_input`), video, display and mode discovery, the
    refresh driver (`sdl_vid`).
  - `win32/` — the remaining Windows pieces: sockets (`net_wins`), console
    helper (`conproc`), shared timing (`q_shwin`), the dynamic video menu
    (`vid_menu`),
    resources (`dnicon.rc`, `q2.ico`).
  - `third-party/` — vendored dependencies, nothing is fetched online:
    `openal-soft` (prebuilt import lib + `soft_oal.dll`, LGPL, linked
    dynamically), `sdl3` (prebuilt import lib + `SDL3.dll`, zlib, linked
    dynamically), `glad` (MIT, compiled into `ref_gl`), `mp3` (dr_mp3, MIT).
- `basedn/` — game data the engine reads at runtime (config, maps,
  textures, sounds). The game modules build directly into it.
- `references/` — reference code kept for study and behavior comparison.
  Nothing here is compiled or linked into the engine.
- `tools/` — build, CI and agent tooling: `vsenv.cmd` (VS environment
  activation), `digest_build_log.py` (CI digest generator),
  `agent-session-notes.md` (durable agent state).
- `.github/workflows/build-windows.yml` — the CI build.
- `PROGRESS.md` — status tracking with the legend.
- `build-digest.md` + `build-windows.log` — committed by CI after every
  push (except `[skip ci]` commits). Read the digest first.
- `duke3d-idtech2.code-workspace` + `.vscode/` — the VS Code setup.

## Building (Windows, llvm.org LLVM)

Requirements:

- LLVM from https://llvm.org/ with the "Add LLVM to the system path"
  installer option. `clang-cl.exe` is the compiler, `lld-link.exe` the
  linker. The Microsoft clang builds are not used.
- Visual Studio or VS Build Tools with the "Desktop development with C++"
  workload. VS only provides headers, libraries, `rc.exe` and the
  debugger; the toolchain itself is LLVM.
- CMake 3.24+, Ninja, Python (only for the opt-in asset pipeline).

Commands (from the repo root):

    call tools\vsenv.cmd x86
    cd code
    cmake --preset windows-clang-x86
    cmake --build --preset windows-clang-x86

The target is x86: the game modules are named `dngamex86`/`cgamex86` and the
vendored OpenAL import lib is picked from `libs/Win32`. Pass
`-DDN_ARCH=x64` to build 64-bit instead.

Outputs: `duke3d.exe` and `ref_gl.dll` at the repo root,
`dngamex86.dll` and `cgamex86.dll` in `basedn/`, `OpenAL32.dll` (copied
from the vendored `soft_oal.dll`) and `SDL3.dll` (copied from the vendored
SDL3 package) next to the executable.

VS Code: open `duke3d-idtech2.code-workspace`, run the `build` task
(Ctrl+Shift+B), F5 to debug. The tasks call `tools/vsenv.cmd` themselves.

## CMake layout

`code/CMakeLists.txt` is a thin root: platform checks, global settings and
one include per project. Every project file is included from the root so
`CMAKE_CURRENT_SOURCE_DIR` stays at `code/` and source paths stay short.

- `code/cmake/utils.cmake` — shared helpers: global compile settings, the
  target-architecture check, output-directory handling, the game-module
  declaration used by `game` and `cgame`.
- `code/cmake/third-party.cmake` — the vendored dependencies and the
  imported `OpenAL` and `SDL3` targets.
- `code/cmake/engine.cmake` — `duke3d.exe`.
- `code/cmake/renderer.cmake` — `ref_gl.dll`.
- `code/cmake/game.cmake` — `dngamex86.dll`.
- `code/cmake/cgame.cmake` — `cgamex86.dll`.
- `code/cmake/assets.cmake` — opt-in asset pipeline; self-deactivates
  while `code/tools/` and the game data are absent.
- `code/cmake/tests.cmake` — the test suite, `BUILD_TESTING=OFF` by
  default until `code/tests/` returns.
- `code/CMakePresets.json` — the `windows-clang-x86` presets.

To add a source file to a module, edit that module's cmake file; do not
grow the root.

## CI and the build digest

Every push (except commits whose message contains `[skip ci]`) builds
Windows x86 with llvm.org clang-cl, then commits `build-digest.md` and
`build-windows.log` to the repo root with a `[skip ci]` message.

Iteration loop:

1. Push the change.
2. Wait for CI to finish. Do not assume success.
3. Read `build-digest.md` at the new HEAD first: errors with context
   snippets, warning kinds with counts, and the log tail are there. Open
   `build-windows.log` only when the digest is not enough.
4. Fix ALL errors AND warnings, surgically. No rewrites, no formatting
   churn; match the surrounding code.
5. Push and repeat until the run is green with zero warnings.

The digest/log commit itself carries `[skip ci]`; never base work on
assuming a build ran against it.

## Conventions

- One branch per change; never commit to `main` directly.
- Update `PROGRESS.md` when the status of an item changes; keep the
  legend: `[ ]` proposed, `[~]` in progress, `[x]` done.
- Keep this guide mirrored in `CLAUDE.md` and
  `.github/copilot-instructions.md`.
- Session state belongs in `tools/agent-session-notes.md`; update it at
  the end of a session instead of leaving context only in chat.
- Build artifacts (`*.exe`, `*.dll`, `*.pdb`) are gitignored; never commit
  them.

## Where to navigate

| Task | Files |
| --- | --- |
| Build a module, add/remove sources | `code/cmake/<module>.cmake` |
| Build failures, warnings | `build-digest.md`, then `build-windows.log` |
| Map loading (Build `.map` format) | `qcommon/dnmap.cpp`, `dnmap.h` |
| Visibility/sector culling | `qcommon/dnvisibility.cpp` |
| CON scripting | `qcommon/dncon.cpp`, `game/dnconvm.cpp`, `game/dnconrun.cpp` |
| Player movement | `qcommon/pmove.cpp`, `qcommon/dnplayermove.cpp` |
| Enemy AI | `game/dnnativeai.cpp`, `game/dnactors.cpp` |
| Sector effects (SE, ST) | `game/dnsectors.cpp` |
| Use/interaction | `game/dnuse.cpp` |
| Weapons | `game/dnweapons.cpp`, `cgame/dnweaponview.cpp`, `dnweaponanim.cpp` |
| Renderer | `ref_gl/gl_rmain.cpp`, `gl_rsurf.cpp`, `dnrender.cpp` |
| Video modes and displays | `sdl/sdl_vid.cpp`, `ref_gl/glimp_sdl.cpp`, `win32/vid_menu.cpp` |
| View fov and aspect | `client/cl_view.cpp` |
| Textures/images | `ref_gl/gl_image.cpp`, `qcommon/dnpcx.cpp` |
| Sound | `sound/sndopenal.cpp`, `sndmusic.cpp`, `third-party/mp3/` |
| Input | `sdl/sdl_input.cpp`, `client/keys.cpp` |
| Network | `server/sv_send.cpp`, `qcommon/net_chan.cpp`, `win32/net_wins.cpp` |
| Dedicated server commands | `server/sv_ccmds.cpp`, `qcommon/cmd.cpp` |
| Save/load | `game/dnsave.cpp`, `client/dnsavemenu.cpp` |
| Platform specifics | `sdl/sdl_sys.cpp`, `sdl/sdl_vid.cpp`, `ref_gl/glimp_sdl.cpp` |
| CI behavior | `.github/workflows/build-windows.yml` |

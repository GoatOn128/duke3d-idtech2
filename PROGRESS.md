# Progress

Legend:

- `[ ]` Proposed — agreed or planned, not started
- `[~]` In progress — being worked on right now
- `[x]` Done — finished and verified (Windows CI green)

## Infrastructure

- [x] CMake refactor: one cmake file per project (`code/cmake/*.cmake`)
      plus shared helpers in `code/cmake/utils.cmake`; the root
      `code/CMakeLists.txt` is a thin orchestrator
- [x] CMake paths fixed for the `code/third-party` layout: OpenAL Soft
      now links from the prebuilt import lib (`third-party/openal-soft`),
      dr_mp3 builds from `third-party/mp3`
- [x] CMake presets for llvm.org Clang (`code/CMakePresets.json`,
      `windows-clang-x86` and its debug variant)
- [x] Windows CI with llvm.org clang-cl that commits `build-digest.md` +
      `build-windows.log` to the repo root (`.github/workflows/build-windows.yml`)
- [x] VS Code workspace configured for LLVM on Windows
      (`duke3d-idtech2.code-workspace`, `.vscode/*`, `tools/vsenv.cmd`)
- [x] Agent documentation: `AGENTS.md`, `CLAUDE.md`,
      `.github/copilot-instructions.md`
- [x] Durable agent tooling under `tools/`
      (`agent-session-notes.md`, `digest_build_log.py`)

## Pending restructure

- [~] Restore the asset pipeline: the `convertgrp` tool
      (`code/tools/convertGRP/convert_grp.c`) converts `3drealms/duke3d.grp`
      into the `basedn/` assets (maps, WAL and UI tiles, palette tables,
      sounds, intro sequence); `code/cmake/assets.cmake` builds it always
      and runs it automatically once the game data is present.
      Still missing: `prepare_references.py` and the reference data
- [ ] Restore the reference-based test suite: `code/tests/` sources and
      the reference sources moving to `references/`; `code/cmake/tests.cmake`
      keeps the full test definitions and turns on once both exist
      (build with `-DBUILD_TESTING=ON` after that)
- [ ] Restore `tests/dnnetwork.cpp`, `tests/dnsave.cpp`,
      `tests/dnroof.cpp` inside the `duke3d` executable (the
      `dn_netcheck`/`dn_savecheck`/`dn_roofcheck` commands)
- [x] Restore the executable icon (`win32/dnicon.rc` compiles the
      in-repo `win32/q2.ico` unconditionally)
- [x] Replace the `win32/qgl_win.cpp` GL loader with `third-party/glad`
      and move `ref_gl` onto an OpenGL 3.3 core profile (GLSL shader
      pair, batched vertex buffers, fixed-function and ancient GPU
      extension paths removed)
- [x] Port the platform layer to SDL3: `sdl/sdl_sys.cpp`,
      `sdl/sdl_input.cpp`, `sdl/sdl_vid.cpp` replace `sys_win`/`in_win`/
      `vid_dll`, `ref_gl/glimp_sdl.cpp` replaces the WGL window code, the
      vendored SDL3 links into `duke3d` and `ref_gl` and ships as
      `SDL3.dll`; the RDTSC timer, CD audio check, alt-tab hooks and other
      pre-Win7 leftovers are removed
- [x] Port the Doom 3 BFG display handling and fov: `sdl/sdl_vid.cpp`
      discovers the displays and their fullscreen modes at runtime,
      `ref_gl/glimp_sdl.cpp` chooses borderless desktop or exclusive
      fullscreen with refresh-rate matching (`gl_displays` command),
      `win32/vid_menu.cpp` builds the video menu from the live lists,
      and `client/cl_view.cpp` widens the fov for widescreen instead
      of stretching the view (`vid_display`, `vid_displayrefresh`,
      custom `gl_mode -1`, `r_aspectRatio`)
- [x] Port the Doom 3 BFG threading foundation: `qcommon/dnthreads.cpp`
      (SDL3 thread/signal/mutex/atomic primitives) and
      `qcommon/dnjobs.cpp` (parallel job lists, threads backend),
      started from `Qcommon_Init` (`jobs_numThreadsRealtime`,
      `jobs_test` command)
- [x] Port the BFG frame step code and uncapped render rate:
      `Qcommon_Frame` clamps the frame delta to `com_deltaTimeClamp`
      like the BFG `Com_Frame` loop, `cl_maxfps 0` renders uncapped
      (new default), and `com_noSleep` / `com_fixedTic` behave like
      their BFG namesakes in `CL_Frame` and `SV_Frame` (the server
      keeps its fixed 100 ms tics, the equivalent of the fixed BFG
      game tics)
- [x] Port the BFG renderer frontend parallelism: the Duke sector mesh
      tessellation (`ref_gl/dnrender.cpp`) is pure CPU work, so the dirty
      sectors retessellate on the phase 1 job threads (and the whole map
      at load), reached through a `ParallelFor` entry in the refresh
      import table (`r_parallelMeshes` toggles it; under 4 sectors the
      builds stay serial)
- [x] Sound threading, closed as a documented divergence: OpenAL
      (`sound/sndopenal.cpp`) mixes on its own thread inside the OpenAL
      implementation and the per-frame sound updates already follow the
      render rate, so a BFG-style dedicated sound thread would duplicate
      existing threading
- [x] Port the BFG console edit field, persistent history and
      cycling tab completion: `qcommon/dneditfield.cpp` (the BFG
      EditField port: insert/overstrike typing, shortest common prefix
      completion that keeps trailing arguments, tab cycling through
      the matches), `qcommon/dnconsolehistory.cpp` (64-entry history
      ring persisted as `consoleHistory.txt` in the gamedir,
      `history`/`clearhistory` commands), and the BFG console key
      handling (`Con_KeyDownEvent`/`Con_Scroll`) in
      `client/console.cpp` replacing the `key_lines` edit ring
- [x] Port the BFG win32 system console and the console debug
      graphs: `win32/dn_syscon.cpp` (the BFG win_syscon port: dedicated
      console window with the history input line, copy/clear/quit
      buttons and the error box, `win_viewlog` replaces the QHOST show
      path), `client/dndebuggraph.cpp` (the BFG DebugGraph port,
      reached through `Con_CreateGraph`), and the `con_frameTimes`
      cvar that renders the rolling frame times like the BFG console
      graphs
- [x] Port the BFG game thread: the server game tic (the game module
      RunFrame plus the Duke world-state apply) runs on its own OS
      thread while `com_smp` is on (`server/sv_gamethread.cpp`, the
      `idGameThread` port from framework/common_frame.cpp;
      `com_sleepGame` debug sleep, inline main-thread tic when
      `com_smp 0`), `qcommon/dnerrortrap.cpp` traps `Com_Error` raised
      during the tic because its longjmp cannot unwind across threads
      and the main thread re-raises it after the wait, and
      `DN_SysThread::SignalWork` clears the done signal before
      signaling new work like the BFG `idSysThread`

## UI port (id Tech 3)

- [x] UI module skeleton: `code/ui/` builds `uix86.dll` with the
      same versioned GetAPI boundary as `cgame`; the entry set follows
      the idTech 3 UI (Init, Shutdown, KeyEvent, MouseEvent, Refresh,
      IsFullscreen, SetActiveMenu, ConsoleCommand, all stubs for now),
      loaded through `client/dnui.cpp` (`CL_InitUI` at startup,
      `CL_UnloadUI` at shutdown)
- [x] Engine-side UI services: `dn_ui_import_t` v2 (cvars, cmd exec,
      file reads, keys and bindings, overstrike, video mode, conn state,
      pic registration and draw, local sound, UpdateScreen) plus the Duke
      menu ownerdraws (UiTile, MenuText, MenuCursor, MenuBackground,
      MenuSound mapping the menu.cpp sounds). The module calls the table
      directly through `uii` like the cgame module with `cg`; no trap
      wrapper layer (the idTech 3 trap indirection was dropped by
      design)
- [x] Port the idTech 3 menu framework (`ui_shared`): the RTCW
      `ui_shared` window/item/menu framework runs in `code/ui/` with
      `ui_util` (print through `uii`, the q_shared helpers the Q2 header
      lacks, the tokenizer), `tr_types.h`/`keycodes.h`/`menudef.h`, and the
      `displayContextDef_t` filled from the engine import table (text and
      pics draw through MenuText/RegisterPic/DrawStretchPic; model, feeder
      and ownerdraw callbacks are stubs until the Duke menus land)
- [x] Level load screen through the UI module (`dn_ui_export_t` v7,
      `DrawLoadScreen` in `ui_main.cpp`, `UI_DrawLoadScreen` in
      `dnui.cpp`): while the client loads a level the screen draws the
      Duke3D load screen, the `dofrontscreens()` layout from the
      original PREMAP.CPP, the LOADSCREEN tile (3281, emitted by
      `convertgrp`) behind LOADING or ENTERING in the selected glyph
      shade and the level name under it, instead of the Quake 2
      console, the idTech 3
      `UI_DRAW_CONNECT_SCREEN` slot; the LOADING variant holds over
      the menu-to-server gap (`scr_load_pending` in `cl_scrn.cpp`),
      the level name arrives with the server data, and the console
      stays only while a download runs
- [x] Menu art and motion as ui data: the qcommon menu parser grows
      named tile and animation tables; `ui/resources.txt` owns the named
      tiles (names follow the original names.h: MENUSCREEN, MENUBAR,
      INGAMEDUKETHREEDEE, SPINNINGNUKEICON, LOADSCREEN),
      `ui/animations.txt` owns the screen slide and the nuke cursor
      spin, and the ui module reads both at init so the menus retarget
      without recompiling
- [x] Customize controls and module config as ui data: the keys screen
      rows come from `ui/binds.lst` ("label" "command" "default key"),
      real labels instead of raw command names, spacing matches the
      Duke menu grid; `ui/config.txt` owns the module behavior values
      (sound channel, focus pulse and blink, save line length); the
      seeded tile/animation defaults are gone so the ui text files are
      the only source; `ui/menus.txt` gained `#include` support
- [x] Ownerdraw rows honor their script rects: the line, tile and row
      ownerdraws paint against the item rect mapped into the Duke menu
      grid (a screen-centered rect keeps the engine centering, left
      anchored rows sit on the rect's left edge, save and server rows
      trim against the right edge, dmflag values right-align on it),
      every .menu ownerdraw item now carries the rect that reproduces
      the original placement, and the code line tables carry only the
      look (text, brightness, cursor behavior); the credits scroller
      keeps ignoring its rect
- [x] Menu inheritance and user script macros: menuDef gained an
      `inherit` keyword that starts a menu from an already loaded one
      (window look, colors, scripts and fade settings carry over, items
      do not), and `ui/gui.txt` owns named script macros the scripts run
      through `gui::<name>` (or `gui <name>`), nesting up to four deep,
      so transitions and repeated script patterns become data
- [x] Dead RTCW constants purged from `code/ui/menudef.h`: the file now
      keeps only what the ported framework and the .menu format use
      (item types, alignments, text styles, borders, window styles,
      range and list box element types); the cgame hud flags, RTCW
      feeder ids, ownerdraw id blocks, voice chats and wolf multiplayer
      classes are gone, and the dead `CG_SHOW_TEXTASINT` cvar text
      check (a popup-flag alias) went with them
- [x] Menu file includes: a `#include <file>` line between menuDef
      blocks splices another .menu file in at that spot, nesting up to
      eight deep like the menus.txt includes; the menu lexer grew an
      unread (a one token push back) so the loader can peek the next
      block opener before handing it to the parser
- [x] UI scripting documentation: `docs/ui-scripting.md` walks through
      the whole menu system the way a person reads it, how
      `ui/menus.txt` loads screens, the .menu syntax with every
      keyword, the script commands including the `duke` subcommands,
      the ownerdraw ids, the feeders, the data files and the
      coordinate mapping, with a worked example screen and the
      gotchas (ownerdraw rows honor their script rects; a missing
      rect paints at the top left corner)
- [ ] Duke menus on the idTech 3 framework (main, new game, options,
      load, save)
- [ ] Cut the menus over to the module, retire `menu.cpp`/`qmenu.cpp`
      from the build

## Engine milestones

- [~] Duke-native map loading: Build `.map` files through
      `qcommon/dnmap.cpp` (no BSP), visibility through
      `qcommon/dnvisibility.cpp`
- [ ] Full gameplay parity with DOS Duke3D (actors, AI, sectors, use)
- [ ] Dedicated server + multiplayer validation

Engine milestone status belongs to the owner; agents should not flip
engine items without a green Windows CI build.

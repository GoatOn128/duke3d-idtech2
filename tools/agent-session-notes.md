# Agent session notes (duke3d-idtech2)

Durable state for AI agents working on this repo. Update at the end of a
session instead of leaving context only in chat. Session wipes should not
cost anything as long as this file and PROGRESS.md are current.

## Owner conventions

- One branch per change; never push to `main` directly.
- Keep agent and CI tooling under `tools/` so it survives.
- `PROGRESS.md` carries status with the legend `[ ]` proposed,
  `[~]` in progress, `[x]` done.
- Engine code: `code/`. Third-party: `code/third-party/`. Reference code
  (not compiled): `references/`.
- Surgical fixes only: no rewrites, no formatting churn.

## Verified tooling constraints (GitHub App connector)

- Fetch repo files ONLY with `get_file_contents`; the content is in
  `resource.text` of the MCP envelope (tolerant extract helper; may need a
  double JSON.parse for a nested envelope). It refuses files over 1MB.
- `push_files` requires the FULL file content. Never push content that came
  from a lossy read.
- Never fetch repo file content through raw.githubusercontent.com or other
  web proxies: they truncate around 32KB and corrupt files.
- Intermittent 502s: retry each connector call up to 6-8 times; avoid
  bursts of parallel calls.
- `search_code` does not index this repo; use directory listings instead.

## CI loop for this repo

- Workflow: `.github/workflows/build-windows.yml`. Builds windows x86 with
  llvm.org clang-cl (VS x86 env for headers/libs only) and commits
  `build-digest.md` + `build-windows.log` at the repo root with `[skip ci]`.
- After every push: fetch `build-digest.md` at the new HEAD (it lists
  errors with context, warning kinds and the log tail), fix ALL errors AND
  warnings, push, repeat until green.
- The digest commit itself carries `[skip ci]`; do not build from it.
- Configure failures show up under "CMake errors" in the digest.
- Stacked PRs are squash-merged, so the merge base stays stale: after a
  lower PR merges, every open PR shows add/add conflicts on the
  bot-committed `build-digest.md`/`build-windows.log` (each branch has
  its own digest content). Fix by deleting those two files from the open
  PR branches with `[skip ci]` commit messages - main keeps its own
  copies fresh via its CI runs, and a branch that does not carry them
  cannot conflict. Then compare the PR diffs (get_diff of the merged and
  the open PR, per-file patch comparison) for any other diverged file and
  sync the branch to main's content. Done 2026-10-28 for PRs #3 and #4:
  digest files deleted, `.github/workflows/build-windows.yml` synced to
  main's vsenv.cmd variant.

## Build facts

- Toolchain: llvm.org LLVM (`clang-cl` + `lld-link`), Ninja generator,
  x86 target (`dngamex86`/`cgamex86` modules, OpenAL import lib from
  `third-party/openal-soft/libs/Win32`).
- Local activation helper: `call tools\vsenv.cmd x86` (VS supplies only
  headers, libs, rc.exe and the debugger).
- Presets: `windows-clang-x86`, `windows-clang-x86-debug` in
  `code/CMakePresets.json`; build dir `code/out/build/x86`.
- Outputs: `duke3d.exe`, `ref_gl.dll`, `OpenAL32.dll`, `SDL3.dll` at
  repo root; `dngamex86.dll`, `cgamex86.dll` in `basedn/`.
- `win32/dnicon.rc` is compiled only when `3drealms/duke3d.ico` exists;
  the missing-asset build intentionally skips it.
- Tests and asset pipeline are dormant until the pending restructure in
  PROGRESS.md lands (`BUILD_TESTING` defaults to OFF).

## Current state

- 2026-10-28: Initial infrastructure landed as stacked branches
  (merge in this order): `cmake-refactor` -> `ci-build-digest` ->
  `vscode-llvm-setup` -> `agent-docs`. CI is green on every branch that
  runs the workflow (`ci-build-digest`, `vscode-llvm-setup`,
  `agent-docs`): configure exit 0, build exit 0, 0 errors, 0 warnings;
  all four binaries link (`duke3d.exe`, `ref_gl.dll`, `dngamex86.dll`,
  `cgamex86.dll`). `cmake-refactor` has no workflow but carries the
  same source tree.
- Zero-warning pass fixes, all synced to every branch:
  - 13 clang-cl strict-mode findings (sndmusic.cpp include path,
    gl_image.cpp paletted-upload return, gl_rsurf.cpp parens,
    cl_tent.cpp switch cases and dangling-else bracing, files.cpp
    user-defined-literal strings, in_win.cpp joystick default return,
    net_wins.cpp address-compare returns, sv_main.cpp and vid_dll.cpp
    explicit void* casts).
  - cl_ents.cpp(1711) fabs -> abs on the integer pmove.origin delta.
  - The `dn_*` debug commands in Qcommon_Init and the
    DN_NetworkTestFrame() hook in CL_Frame are guarded behind the
    DN_TEST_SUPPORT macro; their implementations
    (tests/dnnetwork.cpp, tests/dnsave.cpp, tests/dnroof.cpp) return
    with the test suite restructure, at which point re-add
    `target_compile_definitions(duke3d PRIVATE DN_TEST_SUPPORT)` in
    code/cmake/engine.cmake.
  - The vendored OpenAL32.lib carries no SEH unwind info and lld-link
    enables SafeSEH by default on x86, so the imported OpenAL target
    now carries INTERFACE_LINK_OPTIONS "/SAFESEH:NO" for x86 builds.
- Dormant by design, no action unless asked: asset preparation (needs
  3drealms/duke3d.grp + code/tools/prepare_assets.py), test suite
  (`BUILD_TESTING` OFF until tests/ and 3drealms/ return), and
  win32/dnicon.rc (needs 3drealms/duke3d.ico).

- 2026-10-29: Full SDL3 platform port on branch `port/sdl3-platform`:
  new `code/sdl/` layer (`sdl_sys` entry point, message pump and events,
  `sdl_input`, `sdl_vid`), `ref_gl/glimp_sdl.cpp` replaces
  `glw_imp`/WGL (GL 3.3 core context through SDL_GL, gladLoaderLoadGL),
  `menu.cpp` lost the `win_noalttab` option, `Sys_Milliseconds` derives from
  `SDL_GetTicks`, and `GL_UpdateSwapInterval` uses
  `SDL_GL_SetSwapInterval`. SDL3 is imported by `third-party.cmake`
  and ships as `SDL3.dll` next to `duke3d.exe`. Deleted: `sys_win`,
  `in_win`, `vid_dll`, `glw_imp`, `glw_win.h`, `winquake.h`,
  `client/x86.cpp` (RDTSC timer), `qgl_win.cpp`, the `rw_*` and
  `.rc`/`.aps` leftovers. `win32/` keeps `net_wins`, `conproc`,
  `q_shwin`, `vid_menu`, `dnicon.rc`. SDL3's supported Windows floor
  is Win7+, the new minimum OS for the engine.

- 2026-10-29: Doom 3 BFG display handling + fov port on branch
  `port/glimp-display-fov` (CI green, 0 errors, 0 warnings):
  - `client/ref.h`: `Vid_GetModeInfo` gained a `displayHz` out param;
    `client/vid.h` declares the mode/display list helpers
    (`VID_NumModes`, `VID_ModeData`, `VID_NumDisplays`, `VID_DisplayName`).
  - `sdl/sdl_vid.cpp`: mode list rebuilt from `SDL_GetDisplays` +
    `SDL_GetFullscreenDisplayModes` (32bpp filter, height >= 480 floor,
    unique w/h/hz triples, sorted height/width/hz). New cvars:
    `vid_display`, `vid_displayrefresh`, `vid_customWidth`,
    `vid_customHeight`; `gl_mode -1` = custom resolution.
    Out-of-range `gl_mode` clamps instead of failing.
  - `ref_gl/glimp_sdl.cpp`: BFG-style fullscreen - borderless desktop
    mode when the size matches, exclusive `SDL_SetWindowFullscreenMode`
    with exact refresh matching otherwise; `gl_displays` inventory
    command registered in `R_Register`/`R_Shutdown` (NOT `GLimp_Shutdown`,
    which also runs on window recreation). `vid_display->modified` is
    cleared by `R_SetMode`, observed by `R_BeginFrame` to force a
    vid_ref restart.
  - `win32/vid_menu.cpp`: video menu spinners built from the live lists,
    DISPLAY spinner added, `vid_display` included in the restart
    condition.
  - `client/cl_view.cpp`: BFG CalcFov port (640x480 fov_y reference,
    aspect-widened fov_x, never-shrink clamp; `r_aspectRatio` -1 auto,
    0 = 4:3, 1 = 16:9, 2 = 16:10). CalcFov is non-static and declared
    in `client.h`: `menu.cpp` PlayerConfig_MenuDraw calls it for the
    player preview. CI caught the link error when it was static; the
    BFG clamp path reproduces the old 144x168 preview numbers exactly.

- 2026-10-29: display port follow-up on branch `fix/vid-mode-list-defaults`:
  SDL returns fullscreen modes largest-first and the 64-entry list cap
  kept only the largest resolutions, so the default `gl_mode 3` opened a
  window near the maximum size (and SDL clamps oversized windows, which
  made mode changes in the video menu look like no-ops).
  `VID_MAX_MODES` and the menu cap are now 256 so the whole list survives,
  and `VID_GetModeInfo` resets an off-list `gl_mode` to the default mode
  like BFG resets `r_vidMode`, instead of clamping to the largest entry.
  `ClampMode` in `vid_menu.cpp` mirrors the same policy.

- 2026-10-29: window resize follow-up on `fix/vid-mode-list-defaults`:
  the window is resizable but nothing told the refresh about border
  drags, so the view kept rendering at the old size. `GLimp_BeginFrame`
  now polls `SDL_GetWindowSize` and syncs `vid` plus the client view via
  `ri.Vid_NewWindow` when the size changed (the viewport and the client
  `viddef` follow in `R_BeginFrame`), and `VID_CreateWindow` sets a
  320x240 minimum window size. Runtime only: a vid_restart still returns
  to the selected `gl_mode`.

- 2026-10-29: windowed mode list rework on `fix/vid-windowed-mode-list`,
  after user testing on a Panasonic TV: the display reported no fullscreen
  mode under 1080p, so the mode list was only 1920x1080/4K entries at
  several rates; the default `gl_mode 3` landed on 1920x1080@50 and every
  spinner step changed just the invisible rate. The list now keeps one
  entry per resolution (fastest rate), merges in the common standard
  resolutions that fit the desktop (windowed windows can be any size;
  hz 0 = undiscovered, exclusive fullscreen re-resolves the size), the
  video menu hides the hz for undiscovered entries, and a once-per-start
  check resets a windowed mode that cannot fit the desktop back to the
  default.

- 2026-10-29: BFG threading port, phase 1, on branch `port/bfg-threads-jobs`
  (CI green, 0 errors, 0 warnings): `qcommon/dnthreads.h/.cpp` ports the
  idlib Thread.h primitives onto SDL3 (DN_SysSignal with manual reset,
  DN_SysInterlockedInteger, DN_SysMutex, DN_SysThread worker threads with
  SignalWork/WaitForThread/StopThread), and `qcommon/dnjobs.h/.cpp` ports
  the idParallelJobList_Threads backend merged into one DN_JobList class:
  AddJob, InsertSyncPoint (SYNC_SIGNAL, SYNC_SYNCHRONIZE), Submit/Wait with
  done-marker jobs, fetch-lock work stealing across several job threads,
  a version guard for resubmitted lists, priority-picking DN_JobThread
  workers, and the cvars jobs_numThreadsRealtime/-NonInteractive (-1 auto
  = logical cores - 1). Dropped on purpose: the PS3 SPURS backend, HDD/disk
  jobs, OS thread priority raises. `jobs_test` runs 8192 atomic-counter
  jobs with a sync point and a trailing signal. CI caught that the vendored
  SDL 3.4 renamed SDL_GetCPUCount to SDL_GetNumLogicalCPUCores. Planned
  phases: uncapped fps/step code from framework/common_frame.cpp,
  multithreaded renderer command buffers, sound thread (OpenAL already
  mixes on its own thread, so that phase is low priority).

- 2026-10-29: BFG threading port, phase 2 (uncapped fps / step code), on
  branch `port/bfg-uncapped-fps` (CI green): Qcommon_Frame now clamps the
  frame delta to com_deltaTimeClamp (default 50, like the BFG Com_Frame
  loop in framework/common_frame.cpp) so a hitch cannot run a burst of
  game tics; the new com_fixedTic and com_noSleep behave like their BFG
  namesakes (one game frame per render frame, never skip a frame);
  cl_maxfps 0 now means uncapped and is the new default, and the CL_Frame
  gate also honors com_noSleep/com_fixedTic. SV_Frame skips its realtime
  gate under com_fixedTic. Mapping note: this engine's server already runs
  fixed 100 ms tics (sv.time = framenum * 100), which is the analog of
  the fixed BFG game tics, so com_engineHz has no equivalent here.
  Known caveat, to be handled with the multiplayer milestone: at very high
  frame rates the 64-entry usercmd ring (CMD_BACKUP) can wrap before the
  10 Hz server acks arrive, so vsync or a finite cl_maxfps avoids
  prediction jitter.

- 2026-10-29: BFG threading port, phase 3 (parallel renderer frontend), on
  branch `port/bfg-parallel-render-frontend` (CI green, 0 errors, 0
  warnings): the Duke renderer's per-sector mesh tessellation is pure
  CPU work with no GL calls, so it now runs on the phase 1 job threads -
  one job per dirty sector in R_DukeApplyWorldStates, and the full sector
  list at map load (the JOBLIST_RENDERER_FRONTEND analog). Wiring:
  DN_JobsParallelFor(count, run, userdata) in qcommon/dnjobs.cpp submits
  one job per index to a reusable utility list (DN_JobList gained
  ClearFinishedJobs for reuse); refimport_t gained a ParallelFor entry,
  populated in VID_LoadRefresh. dnrender.cpp: the GLU tess callbacks now
  run on job threads, so the tess statics are thread_local, and errors
  are recorded per sector (dn_meshError_t) and raised on the main thread
  after the wait, because Com_Error cannot unwind from a worker.
  r_parallelMeshes 1 by default; under 4 sectors the builds stay serial
  (the per-job lock overhead beats the tessellation cost). Scoped out on
  purpose: the full BFG frontend/backend command buffer split - ref_gl is
  immediate-mode with an RB_* batching facade, so a faithful port would
  be a renderer rewrite, not a port.

- 2026-10-29: BFG threading port, phase 4 (sound) closed as a documented
  divergence, folded into the phase 3 PR: the engine mixes through OpenAL
  (sound/sndopenal.cpp), which runs its own mixer thread inside the OpenAL
  implementation, and the per-frame sound updates already run once per
  render frame after the phase 2 uncap. A BFG-style dedicated sound
  thread would duplicate existing threading, so no code was added.

- 2026-10-29: BFG console port, phase A (edit field, persistent history,
  cycling tab completion), on branch `port/bfg-console` (CI green, 0
  errors, 0 warnings): qcommon/dneditfield.h/.cpp ports the BFG framework
  EditField (insert/overstrike typing, ctrl-a/e/h/v/c control codes,
  shortest-common-prefix completion that keeps trailing arguments, tab
  cycling through the matches, draw through a caller-supplied character
  callback); qcommon/dnconsolehistory.h/.cpp ports ConsoleHistory
  (64-entry ring with command-prompt up/down pointers, consoleHistory.txt
  persisted in the gamedir, history/clearhistory commands). New
  Cmd_CommandCompletion and Cvar_CommandCompletion enumerate commands,
  aliases and cvars for the completion callbacks. client/console.cpp:
  Con_KeyDownEvent and Con_Scroll replace the Key_Console line editor
  (200 ms first repeat, 100 ms follow-up, keydown polling), Con_DrawInput
  draws the field, Key_ClearTyping clears it, Con_Clear_f jumps to the
  end like the BFG Clear(). client/keys.cpp: the key_lines/key_linepos/
  edit_line/history_line ring and CompleteCommand are gone, Key_Event
  routes key_game/key_console to Con_KeyDownEvent, consolekeys gained
  K_DEL/K_MWHEELUP/K_MWHEELDOWN, and the autorepeat filter only passes
  K_BACKSPACE/K_PAUSE (scroll repeats run on the Con_Scroll timer).
  Q2 merges key and char events, so the split is: printable -> CharEvent,
  ctrl+letter -> control codes, the rest -> field KeyDownEvent; backspace
  maps to CharEvent(K_BACKSPACE), which the field treats as ctrl-h.
  Dropped on purpose: BFG ArgCompletion (no argument completion
  infrastructure in Q2), the con_speed animation (scr_conspeed already
  animates the Q2 console), and the autocomplete highlight bar
  (DrawFill is a palette int, no alpha). License note: the two new
  qcommon modules derive from the BFG framework EditField/ConsoleHistory
  (GPLv3), ported from references/doom3-bfg, which is not compiled.

- 2026-10-29: BFG console port, phase B (win32 system console + console
  debug graphs), on branch `port/bfg-syscon-debuggraph` (CI green, 0
  errors, 0 warnings): win32/dn_syscon.h/.cpp ports the BFG
  sys/win32/win_syscon.cpp at full scope - the dedicated console window
  (blue background, yellow Courier text, copy/clear/quit buttons, the
  error box with the 1 s color polarity timer, WM_SYSCOMMAND SC_CLOSE),
  the subclassed input line with the BFG DIK scan-code key handling
  (history up/down over 64 DN_EditField history lines, tab completion,
  enter echoes "]\n" through Sys_ConsoleOutput and hands the line to
  Qcommon_Frame's Sys_ConsoleInput drain), Conbuf_AppendText in 16 KB
  chunks with the 0x7000 selection-replacement rollover, and
  Win_SetErrorText destroying the input line. sdl_sys.cpp routes
  Sys_ConsoleOutput/Sys_ConsoleInput/Sys_Error through it, the message
  pump runs in Sys_SendKeyEvents, and win_viewlog (the BFG viewlog)
  replaces the QHOST show logic; win32/conproc.cpp (QHOST) is dormant and
  no longer built. client/dndebuggraph.h/.cpp ports the BFG framework
  DebugGraph (grid lines, fill modes, sideways mode, 8x8 labels through
  re.DrawChar, palette colors through re.DrawFill), and console.cpp
  gained the BFG console debug graphs: Con_CreateGraph/Con_DestroyGraph
  (4-slot list) with the con_frameTimes consumer that draws rolling
  frame times (100 px = 100 ms, one grid line at com_deltaTimeClamp,
  bright bars over the clamp), drawn from SCR_UpdateScreen.
  Adaptations: DN_EditField replaces idEditField; the returned syscon
  line carries no newline (Qcommon_Frame adds it; the BFG SE_CONSOLE
  events carried one); the BFG Sys_QueEvent quit path became a
  consoleText append because Q2 has no system event queue; the BFG
  idStr color-code strip in Conbuf_AppendText became a 0x7f high-bit
  mask because Q2 console color lives in the character high bit;
  dn_syscon.h includes nothing because the Q2 common headers are
  unguarded and must land only once per TU (CI caught that with 40
  redefinitions, plus the missing SC_TAB define and a con_frameTimes
  use before its declaration). win_viewlog only shows the console for
  non-dedicated starts; dedicated always shows it, like BFG. License
  note: dn_syscon and dndebuggraph derive from the BFG win_syscon.cpp
  and framework/DebugGraph (GPLv3), ported from references/doom3-bfg,
  which is not compiled.

- 2026-10-29: BFG console port merged to main: PR #20 (phase A) as merge
  commit d208542b, PR #21 (phase B) as squash 6872ed6d. CI pitfall found
  on the #21 merge: a squash merge folds the branch's bot digest and
  chore commits into the merge commit body, and GitHub skips the whole
  post-merge workflow run on main when the pushed head commit message
  contains the skip marker anywhere (the Actions API showed zero runs
  for 6872ed6d, while #20's plain merge commit had none and did run).
  Main's build-digest.md therefore still described d208542b after the
  #21 merge. This follow-up PR retriggered the main build. Rule going
  forward: after merging a branch that carries CI digest commits,
  verify a post-merge run exists (the public Actions API endpoint
  /actions/runs?head_sha=<sha> answers without auth) and prefer a plain
  merge over a squash for such branches.

- 2026-10-29: BFG resolution list auto-fill on branch
  `port/bfg-resolution-autofill` (CI green, 0 errors, 0 warnings): like
  the BFG MenuScreen_Shell_Resolution options are filled from
  R_GetModeListForDisplay instead of a fixed table, sdl_vid.cpp no
  longer merges the hardcoded std_modes standard-resolution table into
  the list - VID_BuildModeList adds only the live SDL fullscreen modes
  (32bpp, >= 480 lines, one entry per resolution keeping the fastest
  rate) plus the desktop mode as a guaranteed valid entry, so the menu
  and gl_mode only ever offer what the display reports; sizes the
  display does not report stay reachable through gl_mode -1 +
  vid_customWidth/vid_customHeight. New VID_DefaultMode() is the reset
  target: largest entry fitting the desktop minus window borders when
  windowed, smallest entry (0) when fullscreen, mirroring the BFG
  r_vidMode reset; VID_GetModeInfo resets an off-list gl_mode through
  it (was: clamp to the largest entry), VID_ValidateWindowedMode resets
  through it, and vid_menu.cpp ClampMode returns it off-list with
  ResetDefaults starting from it. Dropped on purpose: the BFG 60/120 hz
  filter (the list already dedupes per resolution; vid_displayrefresh
  forces other rates) and the BFG per-display submenus (the DISPLAY
  spinner already covers the same list). License note: policy ported
  from the BFG d3xp/menus/MenuScreen_Shell_Resolution.cpp and
  R_GetModeListForDisplay (GPLv3), references/doom3-bfg, not compiled.

- 2026-10-29: BFG threading port, phase D (thread-safe print and error
  system) on branch `port/bfg-threadsafe-print` (CI green, 0 errors, 0
  warnings): like the BFG idLib (framework/Common_printf.cpp) the main
  thread owns the console buffer, the win32 system console window and
  the logfile; common.cpp records the Qcommon_Init thread in
  com_mainThreadId and Com_Printf formats into a local buffer, then
  prints directly on main through Com_MainPrint or queues on a worker
  thread. The queue is a 128x1024 ring behind a DN_SysMutex,
  drop-oldest on overflow with a truncation warning, drained by
  Qcommon_Frame right after the abortframe setjmp. The rd_target
  redirect state is thread_local (rcon/status redirects run inside one
  function scope on one thread). Com_Error off main cannot longjmp into
  abortframe: it records code+message behind a mutex, unwinds through
  the thread_local Com_SetThreadUnwind jump target, and falls back to
  Sys_Error when the thread has no target (the uncaught BFG idException
  analog). Com_TakePendingError hands the record to the owner thread for
  the re-raise; the phase E game thread is the first caller. Documented
  divergence: BFG drops off-main prints to OutputDebugString and Warning
  early-outs; the queue keeps worker output in the console instead.
  qcommon.h gained the four declarations plus the setjmp include.
  License note: policy ported from the BFG framework/Common_printf.cpp
  (GPLv3), references/doom3-bfg, not compiled. Process lesson: pushing a
  from-memory retype of qcommon.h clobbered the header with a different
  codebase's version (CI caught the include-path break); always
  fetch-and-splice in the sandbox, never retype a whole file.

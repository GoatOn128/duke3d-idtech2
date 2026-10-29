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
  `sdl_input`, `sdl_vid`), `ref_gl/glimp_sdl.cpp` replaces `glw_imp`/WGL
  (GL 3.3 core context through SDL_GL, gladLoaderLoadGL), `menu.cpp`
  lost the `win_noalttab` option, `Sys_Milliseconds` derives from
  `SDL_GetTicks`, and `GL_UpdateSwapInterval` uses
  `SDL_GL_SetSwapInterval`. SDL3 is imported by `third-party.cmake`
  and ships as `SDL3.dll` next to `duke3d.exe`. Deleted: `sys_win`,
  `in_win`, `vid_dll`, `glw_imp`, `glw_win.h`, `winquake.h`,
  `client/x86.cpp` (RDTSC timer), `qgl_win.cpp`, the `rw_*` and
  `.rc`/`.aps` leftovers. `win32/` keeps `net_wins`, `conproc`,
  `q_shwin`, `vid_menu`, `dnicon.rc`. SDL3's supported Windows floor
  is Win7+, the new minimum OS for the engine.

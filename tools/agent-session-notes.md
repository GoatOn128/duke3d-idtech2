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

## Build facts

- Toolchain: llvm.org LLVM (`clang-cl` + `lld-link`), Ninja generator,
  x86 target (`dngamex86`/`cgamex86` modules, OpenAL import lib from
  `third-party/openal-soft/libs/Win32`).
- Local activation helper: `call tools\vsenv.cmd x86` (VS supplies only
  headers, libs, rc.exe and the debugger).
- Presets: `windows-clang-x86`, `windows-clang-x86-debug` in
  `code/CMakePresets.json`; build dir `code/out/build/x86`.
- Outputs: `duke3d.exe`, `ref_gl.dll`, `OpenAL32.dll` at repo root;
  `dngamex86.dll`, `cgamex86.dll` in `basedn/`.
- `win32/dnicon.rc` is compiled only when `3drealms/duke3d.ico` exists;
  the missing-asset build intentionally skips it.
- Tests and asset pipeline are dormant until the pending restructure in
  PROGRESS.md lands (`BUILD_TESTING` defaults to OFF).

## Current state

- 2026-10-28: Initial infrastructure landed as stacked branches
  (merge in this order): `cmake-refactor` -> `ci-build-digest` ->
  `vscode-llvm-setup` -> `agent-docs`. First CI run pending on the tip
  branch; iterate on it with the digest loop until green with zero
  warnings.

# Build digest

- Commit: `f4b9a389cfbb4d62724a6e4ab27fca133bd90040`
- Branch: `refgl-gl33`
- Result: configure exit: 0, build exit: -1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40798349972
- Generated: 2026-10-29 01:07:26 UTC

Errors: **11**, warnings: **0**.

## CMake errors

No CMake errors in the log.

## Tool errors

- `lld-link: error: undefined symbol: _gluNewTess`
- `lld-link: error: undefined symbol: _gluTessCallback`
- `lld-link: error: undefined symbol: _gluTessProperty`
- `lld-link: error: undefined symbol: _gluTessNormal`
- `lld-link: error: undefined symbol: _gluTessBeginPolygon`
- `lld-link: error: undefined symbol: _gluTessBeginContour`
- `lld-link: error: undefined symbol: _gluTessVertex`
- `lld-link: error: undefined symbol: _gluTessEndContour`
- `lld-link: error: undefined symbol: _gluTessEndPolygon`
- `lld-link: error: undefined symbol: _gluDeleteTess`
- `lld-link: error: undefined symbol: _gluErrorString`
## Errors

No errors in the log.

## Warnings

No warnings in the log.

## Log tail

```


lld-link: error: undefined symbol: _gluTessEndContour

>>> referenced by CMakeFiles\ref_gl.dir\ref_gl\dnrender.cpp.obj:(void __cdecl BuildMesh(int))



lld-link: error: undefined symbol: _gluTessEndPolygon

>>> referenced by CMakeFiles\ref_gl.dir\ref_gl\dnrender.cpp.obj:(void __cdecl BuildMesh(int))



lld-link: error: undefined symbol: _gluDeleteTess

>>> referenced by CMakeFiles\ref_gl.dir\ref_gl\dnrender.cpp.obj:(void __cdecl BuildMesh(int))



lld-link: error: undefined symbol: _gluErrorString

>>> referenced by CMakeFiles\ref_gl.dir\ref_gl\dnrender.cpp.obj:(void __stdcall TessError(unsigned int))

[125/144] Building CXX object CMakeFiles\duke3d.dir\server\sv_game.cpp.obj
[126/144] Building CXX object CMakeFiles\duke3d.dir\sound\sndmusic.cpp.obj
[127/144] Building CXX object CMakeFiles\duke3d.dir\sound\dnambience.cpp.obj
[128/144] Building CXX object CMakeFiles\duke3d.dir\sound\sndopenal.cpp.obj
[129/144] Building CXX object CMakeFiles\duke3d.dir\third-party\mp3\dnmp3.cpp.obj
ninja: build stopped: subcommand failed.
```


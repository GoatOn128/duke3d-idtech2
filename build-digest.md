# Build digest

- Commit: `6183dce80a64d4bc1e212009a1634a3d60836fd9`
- Branch: `port/bfg-threadsafe-print`
- Result: configure exit: 0, build exit: 1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40885936815
- Generated: 2026-10-29 13:52:54 UTC

Errors: **6**, warnings: **0**.

## CMake errors

No CMake errors in the log.

## Tool errors

No tool-level errors (lld-link, ninja, ...) in the log.

## Errors

Showing the first 6 of 6 error(s).

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\../client\../qcommon/qcommon.h(21),10`: 'q_shared.h' file not found

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\gl_local.h:31:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\../client/ref.h:21:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\../client\../qcommon/qcommon.h(21),10`: 'q_shared.h' file not found

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\gl_local.h:31:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\../client/ref.h:21:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\../client\../qcommon/qcommon.h(21),10`: 'q_shared.h' file not found

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\gl_local.h:31:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\../client/ref.h:21:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\../client\../qcommon/qcommon.h(21),10`: 'q_shared.h' file not found

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\gl_local.h:31:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\../client/ref.h:21:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\../client\../qcommon/qcommon.h(21),10`: 'q_shared.h' file not found

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\gl_local.h:31:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\../client/ref.h:21:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\../client\../qcommon/qcommon.h(21),10`: 'q_shared.h' file not found

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\gl_local.h:31:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\../client/ref.h:21:
  ```


## Warnings

No warnings in the log.

## Log tail

```
FAILED: [code=1] CMakeFiles/ref_gl.dir/ref_gl/gl_model.cpp.obj 
"C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DBUILDING_REF_GL -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -Dref_gl_EXPORTS -ID:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\glad\include -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\sdl3\include /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\ref_gl.dir\ref_gl\gl_model.cpp.obj -clang:-MFCMakeFiles\ref_gl.dir\ref_gl\gl_model.cpp.obj.d /FoCMakeFiles\ref_gl.dir\ref_gl\gl_model.cpp.obj /FdCMakeFiles\ref_gl.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\gl_model.cpp
In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\gl_model.cpp:22:
In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\gl_local.h:31:
In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\../client/ref.h:21:
D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\../client\../qcommon/qcommon.h(21,10): fatal error: 'q_shared.h' file not found
   21 | #include "q_shared.h"
      |          ^~~~~~~~~~~~
1 error generated.
[5/148] Building CXX object CMakeFiles\ref_gl.dir\ref_gl\gl_draw.cpp.obj
FAILED: [code=1] CMakeFiles/ref_gl.dir/ref_gl/gl_draw.cpp.obj 
"C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DBUILDING_REF_GL -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -Dref_gl_EXPORTS -ID:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\glad\include -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\sdl3\include /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\ref_gl.dir\ref_gl\gl_draw.cpp.obj -clang:-MFCMakeFiles\ref_gl.dir\ref_gl\gl_draw.cpp.obj.d /FoCMakeFiles\ref_gl.dir\ref_gl\gl_draw.cpp.obj /FdCMakeFiles\ref_gl.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\gl_draw.cpp
In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\gl_draw.cpp:23:
In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\gl_local.h:31:
In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\../client/ref.h:21:
D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\../client\../qcommon/qcommon.h(21,10): fatal error: 'q_shared.h' file not found
   21 | #include "q_shared.h"
      |          ^~~~~~~~~~~~
1 error generated.
[6/148] Building CXX object CMakeFiles\ref_gl.dir\ref_gl\gl_image.cpp.obj
FAILED: [code=1] CMakeFiles/ref_gl.dir/ref_gl/gl_image.cpp.obj 
"C:\Program Files\LLVM\bin\clang-cl.exe" --target=i686-pc-windows-msvc  /nologo -TP -DBUILDING_REF_GL -DC_ONLY -DWIN32 -D_CRT_NONSTDC_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINDOWS -Dref_gl_EXPORTS -ID:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\glad\include -imsvcD:\a\duke3d-idtech2\duke3d-idtech2\code\third-party\sdl3\include /DWIN32 /D_WINDOWS /EHsc /O2 /Ob2 /DNDEBUG -std:c++17 -MD -Wno-microsoft-anon-tag -Wno-deprecated-declarations -Wno-pragma-pack -Wno-writable-strings -ferror-limit=0 -clang:-MD -clang:-MTCMakeFiles\ref_gl.dir\ref_gl\gl_image.cpp.obj -clang:-MFCMakeFiles\ref_gl.dir\ref_gl\gl_image.cpp.obj.d /FoCMakeFiles\ref_gl.dir\ref_gl\gl_image.cpp.obj /FdCMakeFiles\ref_gl.dir\ -c -- D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\gl_image.cpp
In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\gl_image.cpp:21:
In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\gl_local.h:31:
In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\../client/ref.h:21:
D:\a\duke3d-idtech2\duke3d-idtech2\code\ref_gl\../client\../qcommon/qcommon.h(21,10): fatal error: 'q_shared.h' file not found
   21 | #include "q_shared.h"
      |          ^~~~~~~~~~~~
1 error generated.
ninja: build stopped: subcommand failed.
```


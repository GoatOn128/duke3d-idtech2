# Build digest

- Commit: `8a13d2b8add5d09d1c27227974ee30b37b57914b`
- Branch: `port/sdl3-platform`
- Result: configure exit: 0, build exit: 1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40805194897
- Generated: 2026-10-29 02:15:35 UTC

Errors: **40**, warnings: **0**.

## CMake errors

No CMake errors in the log.

## Tool errors

No tool-level errors (lld-link, ninja, ...) in the log.

## Errors

Showing the first 40 of 40 error(s).

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h(89),16`: redefinition of 'sizebuf_s'

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_input.cpp:24:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h(212),6`: redefinition of 'svc_ops_e'

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_input.cpp:24:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h(246),6`: redefinition of 'clc_ops_e'

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_input.cpp:24:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h(541),5`: redefinition of enumerator 'NA_LOOPBACK'

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_input.cpp:24:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h(542),5`: redefinition of enumerator 'NA_BROADCAST'

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_input.cpp:24:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h(543),5`: redefinition of enumerator 'NA_IP'

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_input.cpp:24:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h(544),5`: redefinition of enumerator 'NA_IPX'

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_input.cpp:24:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h(545),5`: redefinition of enumerator 'NA_BROADCAST_IPX'

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_input.cpp:24:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h(546),3`: typedef redefinition with different types ('enum netadrtype_t' vs 'enum netadrtype_t')

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_input.cpp:24:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h(550),5`: redefinition of enumerator 'NS_CLIENT'

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_input.cpp:24:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h(551),5`: redefinition of enumerator 'NS_SERVER'

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_input.cpp:24:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h(552),3`: typedef redefinition with different types ('enum netsrc_t' vs 'enum netsrc_t')

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_input.cpp:24:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h(562),3`: typedef redefinition with different types ('struct netadr_t' vs 'struct netadr_t')

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_input.cpp:24:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h(617),3`: typedef redefinition with different types ('struct netchan_t' vs 'struct netchan_t')

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_input.cpp:24:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h(40),3`: typedef redefinition with different types ('struct dpackfile_t' vs 'struct dpackfile_t')

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h:642:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h(47),3`: typedef redefinition with different types ('struct dpackheader_t' vs 'struct dpackheader_t')

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h:642:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h(74),3`: typedef redefinition with different types ('struct pcx_t' vs 'struct pcx_t')

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h:642:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h(97),3`: typedef redefinition with different types ('struct dstvert_t' vs 'struct dstvert_t')

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h:642:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h(103),3`: typedef redefinition with different types ('struct dtriangle_t' vs 'struct dtriangle_t')

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h:642:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h(109),3`: typedef redefinition with different types ('struct dtrivertx_t' vs 'struct dtrivertx_t')

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h:642:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h(123),3`: typedef redefinition with different types ('struct daliasframe_t' vs 'struct daliasframe_t')

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h:642:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h(156),3`: typedef redefinition with different types ('struct dmdl_t' vs 'struct dmdl_t')

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h:642:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h(175),3`: typedef redefinition with different types ('struct dsprframe_t' vs 'struct dsprframe_t')

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h:642:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h(183),3`: typedef redefinition with different types ('struct dsprite_t' vs 'struct dsprite_t')

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h:642:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h(195),16`: redefinition of 'miptex_s'

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h:642:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h(254),3`: typedef redefinition with different types ('struct lump_t' vs 'struct lump_t')

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h:642:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h(282),3`: typedef redefinition with different types ('struct dheader_t' vs 'struct dheader_t')

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h:642:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h(291),3`: typedef redefinition with different types ('struct dmodel_t' vs 'struct dmodel_t')

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h:642:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h(296),3`: typedef redefinition with different types ('struct dvertex_t' vs 'struct dvertex_t')

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h:642:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h(315),3`: typedef redefinition with different types ('struct dplane_t' vs 'struct dplane_t')

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h:642:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h(375),3`: typedef redefinition with different types ('struct dnode_t' vs 'struct dnode_t')

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h:642:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h(377),16`: redefinition of 'texinfo_s'

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h:642:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h(391),3`: typedef redefinition with different types ('struct dedge_t' vs 'struct dedge_t')

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h:642:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h(407),3`: typedef redefinition with different types ('struct dface_t' vs 'struct dface_t')

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h:642:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h(424),3`: typedef redefinition with different types ('struct dleaf_t' vs 'struct dleaf_t')

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h:642:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h(430),3`: typedef redefinition with different types ('struct dbrushside_t' vs 'struct dbrushside_t')

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h:642:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h(437),3`: typedef redefinition with different types ('struct dbrush_t' vs 'struct dbrush_t')

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h:642:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h(452),3`: typedef redefinition with different types ('struct dvis_t' vs 'struct dvis_t')

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h:642:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h(461),3`: typedef redefinition with different types ('struct dareaportal_t' vs 'struct dareaportal_t')

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h:642:
  ```

- `D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h(467),3`: typedef redefinition with different types ('struct darea_t' vs 'struct darea_t')

  ```
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
  In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h:642:
  ```


## Warnings

No warnings in the log.

## Log tail

```
  642 | #include "../qcommon/qfiles.h"
      |          ^
D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h(642,10): note: 'D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h' included multiple times, additional include site here
  642 | #include "../qcommon/qfiles.h"
      |          ^
D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h(461,3): note: unguarded header; consider using #ifdef guards or #pragma once
  461 | } dareaportal_t;
      |   ^
In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_input.cpp:24:
In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\sdl_sys.h:4:
In file included from D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h:642:
D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h(467,3): error: typedef redefinition with different types ('struct darea_t' vs 'struct darea_t')
  467 | } darea_t;
      |   ^
D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h(642,10): note: 'D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h' included multiple times, additional include site here
  642 | #include "../qcommon/qfiles.h"
      |          ^
D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon/qcommon.h(642,10): note: 'D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h' included multiple times, additional include site here
  642 | #include "../qcommon/qfiles.h"
      |          ^
D:\a\duke3d-idtech2\duke3d-idtech2\code\sdl\../client\../qcommon\../qcommon/qfiles.h(467,3): note: unguarded header; consider using #ifdef guards or #pragma once
  467 | } darea_t;
      |   ^
40 errors generated.
[115/143] Building CXX object CMakeFiles\duke3d.dir\client\menu.cpp.obj
[116/143] Building CXX object CMakeFiles\duke3d.dir\game\q_shared.cpp.obj
[117/143] Building CXX object CMakeFiles\duke3d.dir\win32\net_wins.cpp.obj
[118/143] Building CXX object CMakeFiles\duke3d.dir\win32\q_shwin.cpp.obj
[119/143] Building C object CMakeFiles\ref_gl.dir\third-party\glad\src\gl.c.obj
ninja: build stopped: subcommand failed.
```


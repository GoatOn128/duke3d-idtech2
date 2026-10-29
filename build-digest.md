# Build digest

- Commit: `2e27bc5f7081b94c39c1bdfbb546a24b9c4a172e`
- Branch: `feat/ui-menu-parser`
- Result: configure exit: 0, build exit: -1
- Actions run: https://github.com/GoatOn128/duke3d-idtech2/actions/runs/40975050774
- Generated: 2026-10-29 23:44:21 UTC

Errors: **16**, warnings: **0**.

## CMake errors

No CMake errors in the log.

## Tool errors

- `lld-link: error: undefined symbol: class dnMenu * __cdecl Menu_GetFocused(void)`
- `lld-link: error: undefined symbol: public: class dnItem * __thiscall dnMenu::GetFocusedItem(void)`
- `lld-link: error: undefined symbol: void __cdecl Controls_GetConfig(void)`
- `lld-link: error: undefined symbol: int __cdecl Menus_AnyFullScreenVisible(void)`
- `lld-link: error: undefined symbol: void __cdecl Menu_PaintAll(void)`
- `lld-link: error: undefined symbol: struct displayContextDef_t * __cdecl Display_GetContext(void)`
- `lld-link: error: undefined symbol: void __cdecl Display_HandleKey(int, int, int, int)`
- `lld-link: error: undefined symbol: int __cdecl Display_MouseMove(void *, int, int)`
- `lld-link: error: undefined symbol: public: void __thiscall dnMenu::Activate(void)`
- `lld-link: error: undefined symbol: class dnMenu * __cdecl Menus_ActivateByName(char const *)`
- `lld-link: error: undefined symbol: public: void __thiscall dnMenu::HandleMouseMove(float, float)`
- `lld-link: error: undefined symbol: public: void __thiscall dnItem::ValidateTypeData(void)`
- `lld-link: error: undefined symbol: class dnItem * __cdecl UI_ConstructItem(int)`
- `lld-link: error: undefined symbol: public: void __thiscall dnItem::Init(void)`
- `lld-link: error: undefined symbol: public: void __thiscall dnItem::FinalizeType(void)`
- `lld-link: error: undefined symbol: public: static class dnMenu * __cdecl dnMenu::Alloc(char const *)`
## Errors

No errors in the log.

## Warnings

No warnings in the log.

## Log tail

```
>>> referenced by CMakeFiles\ui.dir\qcommon\dnmenuparser.cpp.obj:(private: static int __cdecl dnMenuParser::MenuParse_itemDef(class dnItem *, int))



lld-link: error: undefined symbol: public: void __thiscall dnItem::Init(void)

>>> referenced by CMakeFiles\ui.dir\qcommon\dnmenuparser.cpp.obj:(private: static int __cdecl dnMenuParser::MenuParse_itemDef(class dnItem *, int))



lld-link: error: undefined symbol: public: void __thiscall dnItem::FinalizeType(void)

>>> referenced by CMakeFiles\ui.dir\qcommon\dnmenuparser.cpp.obj:(private: static int __cdecl dnMenuParser::MenuParse_itemDef(class dnItem *, int))



lld-link: error: undefined symbol: public: static class dnMenu * __cdecl dnMenu::Alloc(char const *)

>>> referenced by CMakeFiles\ui.dir\qcommon\dnmenuparser.cpp.obj:(private: static int __cdecl dnMenuParser::Menu_New(int))

>>> referenced by CMakeFiles\ui.dir\qcommon\dnmenuparser.cpp.obj:(private: static void __cdecl dnMenuParser::Menu_Load(char const *))

>>> referenced by CMakeFiles\ui.dir\qcommon\dnmenuparser.cpp.obj:(private: static void __cdecl dnMenuParser::Menu_Load(char const *))

[95/155] Building CXX object CMakeFiles\duke3d.dir\client\cl_fx.cpp.obj
[96/155] Building CXX object CMakeFiles\duke3d.dir\client\cl_parse.cpp.obj
[97/155] Building CXX object CMakeFiles\duke3d.dir\client\cl_main.cpp.obj
[98/155] Building CXX object CMakeFiles\duke3d.dir\client\cl_newfx.cpp.obj
[99/155] Building C object CMakeFiles\ref_gl.dir\third-party\glad\src\gl.c.obj
ninja: build stopped: subcommand failed.
```


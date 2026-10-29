# uix86.dll: the id Tech 3 front-end UI module the engine loads from basedn/.
# ui/ui_syscalls.cpp is retired: the module calls the import table directly.
# ui_shared is the RTCW menu framework port; ui_duke_menus and
# ui_duke_menus2 build the Duke menus on it; ui_util supplies the helpers
# the engine Q2 q_shared lacks.

add_library(
  ui SHARED
  ui/ui_main.cpp
  ui/ui_duke_menus.cpp
  ui/ui_duke_menus2.cpp
  ui/ui_shared.cpp
  ui/ui_util.cpp
  game/q_shared.cpp
  ui/ui.def)

duke_declare_game_module(ui uix86)

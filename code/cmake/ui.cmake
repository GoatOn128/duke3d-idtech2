# uix86.dll: the id Tech 3 front-end UI module the engine loads from basedn/.
# ui/ui_syscalls.cpp is retired: the module calls the import table directly.

add_library(
  ui SHARED
  ui/ui_main.cpp
  ui/ui.def)

duke_declare_game_module(ui uix86)

# cgamex86.dll: the client-side game module the engine loads from basedn/.

add_library(
  cgame SHARED
  cgame/dnmain.cpp
  qcommon/dnenemies.cpp
  cgame/dnweaponview.cpp
  qcommon/dnzrange.cpp
  cgame/dnanimation.cpp
  cgame/dnweaponanim.cpp
  qcommon/dnplayermove.cpp
  qcommon/dnmath.cpp
  qcommon/dnanimation.cpp
  qcommon/dncon.cpp
  qcommon/dnmap.cpp
  qcommon/dntransport.cpp
  game/q_shared.cpp
  cgame/cgame.def)

duke_declare_game_module(cgame cgamex86)

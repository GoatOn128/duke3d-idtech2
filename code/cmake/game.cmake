# dngamex86.dll: the server-side game module the engine loads from basedn/.

add_library(
  game SHARED
  game/dnmain.cpp
  game/dnrandom.cpp
  game/dnassets.cpp
  game/dnentity.cpp
  qcommon/dnenemies.cpp
  game/dnlighting.cpp
  game/dnlights.cpp
  game/dndrop.cpp
  game/dnspawn.cpp
  game/dnintermission.cpp
  game/dnplayer.cpp
  game/dnmultiplayer.cpp
  game/dnweapons.cpp
  game/dncombat.cpp
  game/dndamage.cpp
  game/dnprojectiles.cpp
  game/dnactors.cpp
  game/dnalert.cpp
  game/dnphysics.cpp
  qcommon/dnzrange.cpp
  game/dnmove.cpp
  game/dnlaser.cpp
  game/dnenemyshots.cpp
  game/dnnativeai.cpp
  game/dnsound.cpp
  game/dnsectors.cpp
  game/dntransport.cpp
  game/dnstyles.cpp
  game/dndestruct.cpp
  game/dnwallhit.cpp
  game/dnfallers.cpp
  game/dnvehicles.cpp
  game/dneffects.cpp
  game/dndecals.cpp
  game/dngibs.cpp
  game/dnmapspawn.cpp
  game/dnuse.cpp
  game/dnsave.cpp
  game/dncommands.cpp
  qcommon/dncon.cpp
  game/dnconvm.cpp
  game/dnconrun.cpp
  qcommon/dnanimation.cpp
  qcommon/dnmath.cpp
  qcommon/dnplayermove.cpp
  qcommon/dnmap.cpp
  qcommon/dntransport.cpp
  game/q_shared.cpp
  game/game.def)

target_link_libraries(game PRIVATE winmm user32)
duke_declare_game_module(game dngamex86)

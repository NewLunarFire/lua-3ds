#include <citro2d.h>

#include "../lua3.h"

static int lua3game_loop(lua_State *L) {
  int exit = 0;
  int free_render_target = 0;

  C3D_RenderTarget* top = NULL;
  lua_getfield(L, LUA_REGISTRYINDEX, C2D_RENDER_TARGETS);

  if(lua_istable(L, -1)) {
    lua_getfield(L, -1, "top");
    top = lua_touserdata(L, -1);
    lua_pop(L, 1);
  }

  lua_pop(L, 1);

  if(top == NULL) {
    top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    free_render_target = 1;
  }

  while(aptMainLoop() && !exit) {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    hidScanInput();
    C2D_TargetClear(top, C2D_Color32(0, 0, 0, 0xFF));
    C2D_SceneBegin(top);

    lua_getglobal (L, "onRender");
    if(lua_isfunction (L, -1)) {
      lua_call(L, 0, 0); // Call onRender Function
    }

    C3D_FrameEnd(0);

    lua_getfield (L, LUA_REGISTRYINDEX, EXIT_REQUESTED);
    exit = lua_toboolean(L, -1);
    lua_pop(L, 1);
  }

  if(free_render_target) {
    C3D_RenderTargetDelete(top);
  }
  
  return 0;
}

static int lua3game_exit(lua_State *L) {
  lua_pushboolean(L, 1);
  lua_setfield (L, LUA_REGISTRYINDEX, EXIT_REQUESTED);
  
  return 0;
}

static int lua3game_cancel(lua_State *L) {
  lua_pushboolean(L, 0);
  lua_setfield (L, LUA_REGISTRYINDEX, EXIT_REQUESTED);
  
  return 0;
}

int lua3open_game(lua_State *L) {
    lua_pushboolean(L, 0);
    lua_setfield (L, LUA_REGISTRYINDEX, EXIT_REQUESTED);

    lua_newtable(L);

    lua3_setfunction(L, "loop", lua3game_loop);
    lua3_setfunction(L, "exit", lua3game_exit);
    lua3_setfunction(L, "cancel", lua3game_cancel);

    return 1;
}
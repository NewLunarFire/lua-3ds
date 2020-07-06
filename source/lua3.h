#ifndef __LUA3_H__
#define __LUA3_H__

#include <citro2d.h>

#include "lua/lua.h"
#include "lua/lauxlib.h"

#define lua3_setfunction(L, name, func) lua_pushcfunction(L, func); lua_setfield(L, -2, name);

int lua3_runfile (const char* path);

// Preloaded libraries
int lua3open_hid(lua_State *L);
int lua3open_citro2d(lua_State *L);
int lua3open_game(lua_State *L);
int lua3open_cfgu(lua_State *L);

void lua3citro2d_create_rendertargets(lua_State *L);

#define EXIT_REQUESTED "3DSGAME_EXIT_REQUESTED"
#define C2D_RENDER_TARGETS "CITRO2D_RENDER_TARGETS"

#define SCREEN_TOP 0
#define SCREEN_BOTTOM 1

#endif
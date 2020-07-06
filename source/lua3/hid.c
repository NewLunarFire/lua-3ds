#include <stdint.h>
#include <3ds.h>

#include "../lua3.h"

#define lua3hid_setkey(L, n, name, value) lua_pushinteger(L, value);  lua_setfield(L, n, name);

const static uint32_t lua3hid_keyvalues[] = {
    KEY_A,
    KEY_B,
    KEY_SELECT,
    KEY_START,
    KEY_R,
    KEY_L,
    KEY_X,
    KEY_Y,
    KEY_ZL,
    KEY_ZR,
    KEY_TOUCH,
    KEY_DRIGHT,
    KEY_DLEFT,
    KEY_DUP,
    KEY_DDOWN,
    KEY_CSTICK_RIGHT,
    KEY_CSTICK_LEFT,
    KEY_CSTICK_UP,
    KEY_CSTICK_DOWN,
    KEY_CPAD_RIGHT,
    KEY_CPAD_LEFT,
    KEY_CPAD_UP,
    KEY_CPAD_DOWN,
    KEY_RIGHT,
    KEY_LEFT,
    KEY_UP,
    KEY_DOWN
};

const static char* lua3hid_keynames[] = {
    "A",
    "B",
    "SELECT",
    "START",
    "R",
    "L",
    "X",
    "Y",
    "ZL",
    "ZR",
    "TOUCH",
    "DRIGHT",
    "DLEFT",
    "DUP",
    "DDOWN",
    "CSTICK_RIGHT",
    "CSTICK_LEFT",
    "CSTICK_UP",
    "CSTICK_DOWN",
    "CPAD_RIGHT",
    "CPAD_LEFT",
    "CPAD_UP",
    "CPAD_DOWN",
    "RIGHT",
    "LEFT",
    "UP",
    "DOWN"
};

static int lua3hid_keysdown (lua_State *L) {
  lua_pushnumber(L, hidKeysDown());
  return 1;
}

/** Reads the pressed keys from the HID **/
static int lua3hid_keysheld (lua_State *L) {
  lua_pushnumber(L, hidKeysHeld());
  return 1;
}

int lua3open_hid(lua_State *L) {
    lua_newtable(L);

    lua3_setfunction(L, "keysdown", lua3hid_keysdown);
    lua3_setfunction(L, "keysheld", lua3hid_keysheld);

    // Keys table
    lua_newtable(L);
    
    for(size_t i = 0; i < (sizeof(lua3hid_keyvalues) / sizeof(lua3hid_keyvalues[0])); i++) {
        lua_pushinteger(L, lua3hid_keyvalues[i]); 
        lua_setfield(L, -2, lua3hid_keynames[i]);
    }

    lua_setfield(L, -2, "keys");
    return 1;
}
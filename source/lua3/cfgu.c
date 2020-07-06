#include <stdint.h>
#include <3ds.h>

#include "../lua3.h"

#define ARRAY_LENGTH(a) (sizeof(a) / sizeof(a[0]))

const char* language_names[] = {"Japanese", "English", "French", "Deutsch", "Italian", "Spanish", "Chinese", "Korean", "Dutch", "Portugese", "Russian", "Taiwanese"};
const char* region_names[] = {"Japan", "North America", "Europe", "Australia", "China", "Korea", "Taiwan"};
const char* model_names[] = {"3DS", "3DS XL", "New 3DS", "2DS", "New 3DS XL", "New 2DS XL"};

static int lua3cfgu_getRegion(lua_State *L) {
    u8 region;
    bool toString = true;

    if(lua_gettop(L) >= 1 && lua_isboolean(L, 1)) {
        toString = lua_toboolean(L, 1);
    }

    CFGU_SecureInfoGetRegion (&region);

    if(toString) {
        if(region >= ARRAY_LENGTH(region_names)) {
            lua_pushstring(L, "Unknown");
        } else {
            lua_pushstring(L, region_names[region]);
        }
    } else {
        lua_pushinteger(L, region);
    }
    
    return 1;
}

static int lua3cfgu_getLanguage(lua_State *L) {
    u8 language;
    bool toString = true;

    if(lua_gettop(L) >= 1 && lua_isboolean(L, 1)) {
        toString = lua_toboolean(L, 1);
    }

    CFGU_GetSystemLanguage (&language);

    if(toString) {
        if(language >= ARRAY_LENGTH(language_names)) {
            lua_pushstring(L, "Unknown");
        } else {
            lua_pushstring(L, language_names[language]);
        }
    } else {
        lua_pushinteger(L, language);
    }

    return 1;
}

static int lua3cfgu_getModel(lua_State *L) {
    u8 model;
    bool toString = true;

    if(lua_gettop(L) >= 1 && lua_isboolean(L, 1)) {
        toString = lua_toboolean(L, 1);
    }

    CFGU_GetSystemModel(&model);

    if(toString) {
        if(model >= ARRAY_LENGTH(model_names)) {
            lua_pushstring(L, "Unknown");
        } else {
            lua_pushstring(L, model_names[model]);
        }
    } else {
        lua_pushinteger(L, model);
    }

    return 1;
}

static int lua3cfgu_isNFCSupported(lua_State *L) {
    bool isSupported;
    CFGU_IsNFCSupported (&isSupported);
    lua_pushboolean(L, isSupported);

    return 1;
}

int lua3open_cfgu(lua_State *L) {
    lua_newtable(L);

    lua3_setfunction(L, "getRegion", lua3cfgu_getRegion);
    lua3_setfunction(L, "getLanguage", lua3cfgu_getLanguage);
    lua3_setfunction(L, "getModel", lua3cfgu_getModel);
    lua3_setfunction(L, "isNFCSupported", lua3cfgu_isNFCSupported);

    return 1;
}
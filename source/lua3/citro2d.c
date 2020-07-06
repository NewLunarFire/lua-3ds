#include <citro2d.h>

#include "../lua3.h"

static int lua3citro2d_createTint(lua_State *L) {
  C2D_ImageTint* tint;
  double blend = 1.0;
  u32 color;

  int n = lua_gettop(L);

  if(n < 1) {
    return 0;
  }

  color = lua_tointeger(L, 1);
  if(n >= 2) {
    blend = lua_tonumber(L, 2);
  }

  tint = lua_newuserdata(L, sizeof(C2D_ImageTint));
  C2D_PlainImageTint(tint, color, blend);

  return 1;
}

static int lua3citro2d_spriteSheetGetImage(lua_State *L) {
  C2D_SpriteSheet sheet;
  C2D_Image* image;
  int index;

  if(lua_gettop(L) != 2 ) {
    return 0;
  }

  sheet = lua_touserdata(L, 1);
  index = lua_tointeger(L, 2);

  image = lua_newuserdata(L, sizeof(C2D_Image));
  *image = C2D_SpriteSheetGetImage (sheet, index - 1);

  return 1;
}

static int lua3citro2d_loadSpriteSheet(lua_State *L) {
  C2D_SpriteSheet sheet;

  if(lua_gettop(L) != 1 ) {
    return 0;
  }

  if(!lua_isstring(L, 1)) {
    return 0;
  }

  sheet = C2D_SpriteSheetLoad (lua_tostring(L, 1));
  lua_pushlightuserdata(L, sheet);
  
  return 1;
}

static int lua3citro2d_chooseScreen(lua_State *L) {
  const char* screen;
  C3D_RenderTarget* target = NULL;

  if(lua_gettop(L) != 1) {
    return 0;
  }

  if(!lua_isstring(L, 1)) {
    return 0;
  }

  screen = lua_tostring(L, 1);
  lua_getfield(L, LUA_REGISTRYINDEX, C2D_RENDER_TARGETS);
  if(!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return 0;
  }
  
  lua_getfield(L, -1, screen);
  target = lua_touserdata(L, -1);

  if(target != NULL) {
    C2D_TargetClear(target, C2D_Color32(0, 0, 0, 0xFF));
    C2D_SceneBegin(target);
  }

  lua_pop(L, 2);
  return 0;
}

static int lua3citro2d_parseColor(lua_State *L) {
    u8 r, g, b, a = 255;
    int n = lua_gettop(L);

    if(n == 1) {
        // 1 argument

        if(lua_isnumber(L, 1)) {
            // Numerical argument, make sure it is an integer
            lua_pushvalue(L, 1);
            lua_tointeger(L, -1);

            return 1;
        }

        else if(lua_istable(L, 1)) {
            // Table, get r,g,b,a fields

            lua_getfield(L, 1, "r");
            r = lua_tointeger(L, 2);
            lua_pop(L, 1);

            lua_getfield(L, 1, "g");
            g = lua_tointeger(L, 2);
            lua_pop(L, 1);

            lua_getfield(L, 1, "b");
            b = lua_tointeger(L, 2);
            lua_pop(L, 1);

            lua_getfield(L, 1, "a");
            if(lua_isinteger(L, -1)) {
                a = lua_tointeger(L, -1);
                lua_pop(L, 1);
            }

            lua_pushinteger(L, C2D_Color32(r, g, b, a));
            return 1;
        }
    } else if(n == 4) {
        r = lua_tointeger(L, 1);
        g = lua_tointeger(L, 2);
        b = lua_tointeger(L, 3);

        if(lua_isinteger(L, 4)) {
            a = lua_tointeger(L, 4);
        }

        lua_pushinteger(L, C2D_Color32(r, g, b, a));
        return 1;
    }
    
    return 0;
}

static int lua3citro2d_drawImage(lua_State *L) {
  C2D_Image* img;
  C2D_ImageTint* tint = NULL;
  float x = 0, y = 0, depth = 0, angle = 0, scale = 1.0;

  int n = lua_gettop(L);
  
  if(n < 1) {
    // No image
    return 0;
  }

  img = lua_touserdata(L, 1);

  switch(n) {
      default:
      case 7:
        angle = lua_tonumber(L, 7);
      case 6:
        tint = lua_touserdata(L, 6);
      case 5:
        depth = lua_tonumber(L, 5);
      case 4:
        scale = lua_tonumber(L, 4);
      case 3:
        y = lua_tonumber(L, 3) + ((float)img->subtex->height / 2);
      case 2:
        x = lua_tonumber(L, 2) + ((float)img->subtex->width / 2);
  }

  

  C2D_DrawImageAtRotated (*img, x, y, depth, angle, tint, scale, scale);
  return 0;
}

static int lua3citro2d_drawText(lua_State *L) {
  C2D_Text text;
  C2D_TextBuf buffer;
  const char* string;
  double x = 0.0f, y = 0.0f, scale = 1.0f, depth = 0.0f;
  u32 color = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);

  int n = lua_gettop(L);
  
  if(n < 1) {
    // No argument
    return 0;
  }

  if(!lua_isstring (L, 1)) {
    // Argument is not string
    return 0;
  }

  switch(n) {
      default:
      case 6:
        lua_pushcfunction(L, lua3citro2d_parseColor);
        lua_pushvalue(L, 6);
        lua_call(L, 1, 1);
        color = lua_tointeger(L, -1);
        lua_pop(L, 1);
      case 5:
        depth = lua_tonumber(L, 5);
      case 4:
        scale = lua_tonumber(L, 4);
      case 3:
        y = lua_tonumber(L, 3);
      case 2:
        x = lua_tonumber(L, 2);
      case 1:
        break;
  }

  string = lua_tostring(L, 1);
  buffer = C2D_TextBufNew(strlen(string) + 1);
  C2D_TextParse(&text, buffer, string);
  C2D_DrawText(&text, C2D_WithColor, x, y, depth, scale, scale, color);
  C2D_TextBufDelete(buffer);

  return 0;
}

static int lua3citro2d_drawRect(lua_State *L) {
  u32 c[4] = { 0, 0, 0, 0 };

  int n = lua_gettop(L);
  if(n < 5) {
    return 0;
  }

  float x = lua_tonumber(L, 1);
  float y = lua_tonumber(L, 2);
  float width = lua_tonumber(L, 3);
  float height = lua_tonumber(L, 4);

  for(int i = 5; i < (n > 8 ? 8 : n); i++) {
    lua_pushcfunction(L, lua3citro2d_parseColor);
    lua_pushvalue(L, i);
    lua_call(L, 1, 1);
    c[i-5] = lua_tointeger(L, -1);
    lua_pop(L, 1);
  }

  if(n == 5) {
    C2D_DrawRectSolid (x, y, 1.0f, width, height, c[0]);
  } else {
    C2D_DrawRectangle (x, y, 1.0f, width, height, c[0], c[1], c[2], c[3]);
  }

  return 0;
}

static int lua3citro2d_drawTriangle(lua_State *L) {
  float depth = 0;
  u32 c[3] = { 0, 0, 0 };
  int n_colors = 1;
  
  int n = lua_gettop(L);

  if(n < 7) {
    return 0;
  } if (n >= 9) {
    n_colors = 3;
  }

  float x0 = lua_tonumber(L, 1);
  float y0 = lua_tonumber(L, 2);
  
  float x1 = lua_tonumber(L, 3);
  float y1 = lua_tonumber(L, 4);
  
  float x2 = lua_tonumber(L, 5);
  float y2 = lua_tonumber(L, 6);

  for(int i = 0; i < n_colors; i++) {
    lua_pushcfunction(L, lua3citro2d_parseColor);
    lua_pushvalue(L, i+7);
    lua_call(L, 1, 1);
    c[i] = lua_tointeger(L, -1);
    lua_pop(L, 1);
  }

  if(n_colors == 1) {
    c[1] = c[0];
    c[2] = c[0];
  }

  if(n == 8) {
    depth = lua_tonumber(L, 8);
  } else if(n == 8) {
    depth = lua_tonumber(L, 10);
  }
  
  C2D_DrawTriangle(x0, y0, c[0], x1, y1, c[1], x2, y2, c[2], depth);

  return 0;
}

static int lua3citro2d_drawCircle(lua_State *L) {
  float depth = 0;
  u32 c[4] = { 0, 0, 0, 0 };
  int n_colors = 1;
  
  int n = lua_gettop(L);

  if(n < 4) {
    return 0;
  } if (n >= 7) {
    n_colors = 4;
  }

  float x = lua_tonumber(L, 1);
  float y = lua_tonumber(L, 2);
  
  float radius = lua_tonumber(L, 3);

  for(int i = 0; i < n_colors; i++) {
    lua_pushcfunction(L, lua3citro2d_parseColor);
    lua_pushvalue(L, i+4);
    lua_call(L, 1, 1);
    c[i] = lua_tointeger(L, -1);
    lua_pop(L, 1);
  }

  if(n == 5) {
    depth = lua_tonumber(L, 5);
  } else if(n == 8) {
    depth = lua_tonumber(L, 8);
  }
  
  if(n_colors == 1) {
    C2D_DrawCircleSolid(x, y, depth, radius, c[0]);
  } else if(n_colors == 4) {
    C2D_DrawCircle 	(x, y, depth, radius, c[0], c[1], c[2], c[3]);
  }

  return 0;
}

static int lua3citro2d_drawEllipse(lua_State *L) {
  float depth = 0;
  u32 c[4] = { 0, 0, 0, 0 };
  int n_colors = 1;
  
  int n = lua_gettop(L);

  if(n < 5) {
    return 0;
  } if (n >= 8) {
    n_colors = 4;
  }

  float x = lua_tonumber(L, 1);
  float y = lua_tonumber(L, 2);
  
  float width = lua_tonumber(L, 3);
  float height = lua_tonumber(L, 4);

  for(int i = 0; i < n_colors; i++) {
    lua_pushcfunction(L, lua3citro2d_parseColor);
    lua_pushvalue(L, i+5);
    lua_call(L, 1, 1);
    c[i] = lua_tointeger(L, -1);
    lua_pop(L, 1);
  }

  if(n == 6) {
    depth = lua_tonumber(L, 6);
  } else if(n == 9) {
    depth = lua_tonumber(L, 9);
  }
  
  if(n_colors == 1) {
    C2D_DrawEllipseSolid(x, y, depth, width, height, c[0]);
  } else if(n_colors == 4) {
    C2D_DrawEllipse(x, y, depth, width, height, c[0], c[1], c[2], c[3]);
  }

  return 0;
}

int lua3citro2d_rendertargets__gc(lua_State *L) {
  C3D_RenderTarget *top, *bottom;

  lua_getfield(L, 1, "top");
  top = lua_touserdata(L, -1);
  lua_getfield(L, 1, "bottom");
  bottom = lua_touserdata(L, -1);

  C3D_RenderTargetDelete(top);
  C3D_RenderTargetDelete(bottom);
  
  lua_pop(L, 2);
  return 0;
}

void lua3citro2d_create_rendertargets(lua_State *L) {
  C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
  C3D_RenderTarget* bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

  // Rendertargets table
  lua_newtable(L);

  lua_pushlightuserdata(L, top);
  lua_setfield(L, -2, "top");

  lua_pushlightuserdata(L, bottom);
  lua_setfield(L, -2, "bottom");

  // Render targets metatable
  lua_newtable(L);
  lua_pushcfunction(L, lua3citro2d_rendertargets__gc);
  lua_setfield(L, -2, "__gc");

  // Set as render targets metatable
  lua_setmetatable(L, -2);
  
  // Save in registry
  lua_setfield(L, LUA_REGISTRYINDEX, C2D_RENDER_TARGETS);
}

int lua3open_citro2d(lua_State *L) {
    lua3citro2d_create_rendertargets(L);
    
    // Module table
    lua_newtable(L);

    // Function to switch between top and bottom screens
    lua3_setfunction(L, "chooseScreen", lua3citro2d_chooseScreen);

    // Color functions
    lua3_setfunction(L, "parseColor", lua3citro2d_parseColor);
    lua3_setfunction(L, "createTint", lua3citro2d_createTint);

    // Drawing functions
    lua3_setfunction(L, "drawRect", lua3citro2d_drawRect);
    lua3_setfunction(L, "drawTriangle", lua3citro2d_drawTriangle);
    lua3_setfunction(L, "drawCircle", lua3citro2d_drawCircle);
    lua3_setfunction(L, "drawEllipse", lua3citro2d_drawEllipse);
    lua3_setfunction(L, "drawText", lua3citro2d_drawText);
    lua3_setfunction(L, "drawImage", lua3citro2d_drawImage);

    // Spritesheet
    lua3_setfunction(L, "loadSpriteSheet", lua3citro2d_loadSpriteSheet);
    lua3_setfunction(L, "spriteSheetGetImage", lua3citro2d_spriteSheetGetImage);

    return 1;
}
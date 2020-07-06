/** Lua3.c
** The file contains the lua functions used in lua-3ds
** Similar to how lua.c was, except without cli arguments parsing
** and with specific 3DS stuff
**/
#include "lua/lprefix.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h>
#include <citro2d.h>

#include <unistd.h>

#include "lua/lua.h"

#include "lua/lauxlib.h"
#include "lua/lualib.h"

#include "lua3.h"

#include "lrdb/lrdb.h"

int lua3_loadFile(lua_State *L, const char* path, const char* name);

static inline void unicodeToChar(char* dst, u16* src, int max)
{
	if(!src || !dst)return;
	int n=0;
	while(*src && n<max-1){*(dst++)=(*(src++))&0xFF;n++;}
	*dst=0x00;
}

static int listDirectory(lua_State *L) {
  Handle dirHandle;
  FS_Archive sdmcArchive;
  FS_DirectoryEntry entry;
  u32 entriesRead = 0;

  char filename[1024];
  unsigned int n = 1;

  if(lua_gettop(L) < 1) {
    return 0;
  }

  const char* directory = lua_tostring(L, 1);

  lua_newtable(L);
  int table_index = lua_gettop(L);

  // Open SDMC
  FSUSER_OpenArchive(&sdmcArchive, ARCHIVE_SDMC, (FS_Path){PATH_EMPTY, 1, (u8*)""});

  // Scan files
	FSUSER_OpenDirectory(&dirHandle, sdmcArchive, fsMakePath(PATH_ASCII, directory));

  do 
  {
    // Set memory to zero
    memset(filename, 0, 1024);
    memset(&entry,0,sizeof(FS_DirectoryEntry));

    // Read directory
    FSDIR_Read(dirHandle, &entriesRead, 1, &entry);

    if(entriesRead)
    {
      lua_pushinteger(L, n++);
      unicodeToChar(filename, entry.name, 1024);

      lua_newtable(L);
      int entry_index = lua_gettop(L);
      
      // Copy filename
      lua_pushstring(L, "name");
      lua_pushstring(L, filename);
      lua_settable(L, entry_index);

      // Add file size
      lua_pushstring(L, "size");
      lua_pushinteger(L, entry.fileSize);
      lua_settable(L, entry_index);

      // Add attributes
      lua_pushstring(L, "attributes");

      lua_newtable(L);
      int attr_index = lua_gettop(L);

      lua_pushstring(L, "directory");
      lua_pushboolean(L, entry.attributes & FS_ATTRIBUTE_DIRECTORY ? 1 : 0);
      lua_settable(L, attr_index);

      lua_pushstring(L, "hidden");
      lua_pushboolean(L, entry.attributes & FS_ATTRIBUTE_HIDDEN ? 1 : 0);
      lua_settable(L, attr_index);

      lua_pushstring(L, "archive");
      lua_pushboolean(L, entry.attributes & FS_ATTRIBUTE_ARCHIVE ? 1 : 0);
      lua_settable(L, attr_index);

      lua_pushstring(L, "readonly");
      lua_pushboolean(L, entry.attributes & FS_ATTRIBUTE_READ_ONLY ? 1 : 0);
      lua_settable(L, attr_index);

      // Push the attributes into the entry table
      lua_settable(L, entry_index);

      // Push the entry into the file list
      lua_settable(L, table_index);
    }
  } while (entriesRead);

	FSDIR_Close(dirHandle);
  FSUSER_CloseArchive(sdmcArchive);

  return 1;
}

/** Register the 3DS specific functions in the current Lua state **/
void lua3_preload(lua_State *L) {
  luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);

  lua_pushcfunction(L, lua3open_hid); lua_setfield(L, -2, "hid");
  lua_pushcfunction(L, lua3open_citro2d); lua_setfield(L, -2, "citro2d");
  lua_pushcfunction(L, lua3open_game); lua_setfield(L, -2, "game");
  lua_pushcfunction(L, lua3open_cfgu); lua_setfield(L, -2, "cfgu");

  lua_pop(L, 1);
}

static int runScript(lua_State *L) {
  int lrdb = 0;
  
  int n = lua_gettop(L);
  if(n < 1) {
    return 0;
  }

  const char* path = lua_tostring(L, 1);
  const char* scriptName = lua_tostring(L, 2);
  lua_State *L2 = luaL_newstate();  /* create new state */
  if (L2 == NULL) {
    return 0; // Exit failure
    // We should return a status code + an optional error message just in case
  }

  if(n >= 2) {
    if(strcmp(lua_tostring(L, 3), "lrdb") == 0) {
      lrdb = 1; // LRDB selected as debugger
    }
  }

  luaL_openlibs(L2);  /* open standard libraries */
  lua3_preload(L2); /* Open 3DS functions */

  chdir(path);
  int status = luaL_loadfile(L2, scriptName);

  if(status != LUA_OK) {
    // Load failed
    lua_pushinteger(L, status);
    lua_pushstring(L, lua_tostring(L2, -1));

    lua_close(L2);
    return 2;
  }

  // Delete render targets and garbage collect them
  lua_pushnil(L);
  lua_setfield(L, LUA_REGISTRYINDEX, C2D_RENDER_TARGETS);
  lua_gc(L, LUA_GCCOLLECT, 0);

  if(lrdb) lrdb_register(L2);
  status = lua_pcall(L2, 0, 0, 0); /* Call script in protected mode */
  if(lrdb) lrdb_close(L2);

  lua3citro2d_create_rendertargets(L);

  if(status != LUA_OK) {
    // Load failed
    lua_pushinteger(L, status);
    lua_pushstring(L, lua_tostring(L2, -1));
  } else {
    // All good
    lua_pushinteger(L, status);
    lua_pushnil(L);
  }
  
  lua_close(L2); /* Close run state */
  return 2;
}

/** Loads an entire file into RAM and compile it as a Lua script
** We could do better than this, by using an actual loader and reading the file by chunks
** But since the files are small enough to fit in RAM, why bother for now **/
int lua3_loadFile(lua_State *L, const char* path, const char* chunkname) {
    char* buffer;
    unsigned int length;

    FILE* f = fopen(path, "r");
    
    if(!f) {
      return -1;
    }

    fseek(f, 0, SEEK_END); // seek to end of file
    length = ftell(f); // get current file pointer
    fseek(f, 0, SEEK_SET); // seek back to beginning of file
    buffer = malloc(length); // allocate buffer
    fread (buffer, sizeof(char), length, f); // read file into buffer
    fclose(f); // close

    return luaL_loadbuffer(L, buffer, length, chunkname);
    
    free(buffer);
}

static int lua3_pmain(lua_State *L) {
  luaL_openlibs(L);  /* open standard libraries */
  lua3_preload(L); /* Open 3DS functions */

  // Register additional functions for the file manager
  lua_register(L, "listDirectory", listDirectory);
  lua_register(L, "runScript", runScript);

  lua_call(L, 0, 0);

  return 0;
}

void waitForStart() {
  while(aptMainLoop()) {
    hidScanInput();
    u32 keys = hidKeysDown();
    if(keys & KEY_START) {
      break;
    }
  }
}

/** Runs a file at the specified path */
int lua3_runfile (const char* path) {
  int status;

  lua_State *L = luaL_newstate();  /* create state */
  if (L == NULL) {
    return EXIT_FAILURE;
  }

  lua_pushcfunction(L, lua3_pmain); /* Push pmain function */
  status = lua3_loadFile(L, path, "=script"); /* Load script */
  
  if(status == LUA_OK) {
    status = lua_pcall(L, 1, 0, 0); /* Call script in protected mode */

    if(lua_gettop(L) > 0)  {
      if(lua_isstring(L, 1)) {
        printf("Script error: %s\nPress Start to exit\n", lua_tostring(L, 1));
        waitForStart();
      }
    }

  }
  
  lua_close(L);
  return status;
}
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>

#include <unistd.h> 
#include <sys/socket.h> 
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include <3ds.h>

#include "../lua/lua.h"
#include "../lua/lauxlib.h"

#include "lrdb.h"
#include "util.h"

#include "tiny-json.h"
#include "json-maker.h"

#define FAILURE 0
#define SUCCESS 1
#define MAX_FIELDS 32

#define LRDB_PORT 21110
#define SOC_ALIGN 0x1000
#define SOC_BUFFERSIZE 0x100000

char* lrdb_write_value2(lua_State *L, char* json, const char* name, int recurse);

void lrdb_return_null(int socket, int id) {
  char event[64] = {0};
  sprintf(event, "{\"id\":%d,\"jsonrpc\":\"2.0\",\"result\":null}\r\n", id);
  send(socket, event, strlen(event), 0);
}

void lrdb_connected(int socket) {
  char* event = "{\"jsonrpc\":\"2.0\",\"method\":\"connected\",\"params\":{\"lua\":{\"copyright\":\"Lua 5.3.5  Copyright (C) 1994-2018 Lua.org, PUC-Rio\",\"release\":\"Lua 5.3.5\",\"version\":\"Lua 5.3\"},\"protocol_version\":\"2\"}}\r\n";
  send(socket, event, strlen(event), 0); 
}

void lrdb_running(int socket) {
  char* event = "{\"jsonrpc\":\"2.0\",\"method\":\"running\"}\r\n";
  send(socket, event, strlen(event), 0); 
}

void lrdb_paused(int socket, const char* reason) {
  char event[128];
  sprintf(event, "{\"jsonrpc\":\"2.0\",\"method\":\"paused\",\"params\":{\"reason\":\"%s\"}}\r\n", reason);
  send(socket, event, strlen(event), 0); 
}

void lrdb_get_stack_trace(lua_State *L, LRDB_Data* lrdb_data, LRDB_Command* lrdb_command) {
  lua_Debug ar;
  int level = 0;
  char payload[1024] = {0};

  char* end = json_objOpen(payload, NULL);
  end = json_str(end, "jsonrpc", "2.0");
  end = json_ulong(end, "id", lrdb_command->id);
  end = json_arrOpen(end, "result");

  while(lua_getstack(L, level++, &ar) != 0) {
    lua_getinfo(L, "nSl", &ar);

    end = json_objOpen(end, NULL);
    end = json_str(end, "file", ar.source);
    end = json_str(end, "func", ar.name != NULL ? ar.name : ar.what);
    end = json_str(end, "id", ar.what[0] == 'C' ? "[C]" : &ar.source[1]);
    end = json_ulong(end, "line", ar.currentline);
    end = json_objClose(end);
  }

  end = json_arrClose(end);
  end = json_objClose(end);
  end = json_end(end);

  end[0] = '\r';
  end[1] = '\n';

  send(lrdb_data->fd, payload, (end - payload) + 2, 0);
}

void lrdb_get_local_variables(lua_State *L, LRDB_Data* lrdb_data, LRDB_Command* lrdb_command) {
  char payload[16384] = {0};
  lua_Debug ar;
  char* end = json_objOpen(payload, NULL);
  
  end = json_str(end, "jsonrpc", "2.0");
  end = json_ulong(end, "id", lrdb_command->id);
  end = json_objOpen(end, "result");

  lua_getstack(L, 0, &ar);

  int i = 1;
  const char* name = NULL;

  do {
    name = lua_getlocal (L, &ar, i++);
    if(name != NULL) {
      if(strcmp("(*temporary)", name) != 0) {
        end = lrdb_write_value2(L, end, name, 1);
      }

      lua_pop(L, 1);
    }
    
  } while(name != NULL);
  
  end = json_objClose(end);
  end = json_objClose(end);
  end = json_end(end);
  end[0] = '\n';

  send(lrdb_data->fd, payload, (end - payload) + 1, 0);
}

void lrdb_get_upvalues(lua_State *L, LRDB_Data* lrdb_data, LRDB_Command* lrdb_command) {
  char payload[16384] = {0};
  int i = 1;
  lua_Debug ar;
  const char* name = NULL;
  char* end = json_objOpen(payload, NULL);
  
  end = json_str(end, "jsonrpc", "2.0");
  end = json_ulong(end, "id", lrdb_command->id);
  end = json_objOpen(end, "result");
  
  lua_getstack(L, 0, &ar);
  lua_getinfo(L, "f", &ar);

  do {
    const char* name = lua_getupvalue (L, -1, i++);
    if(name != NULL) {
      printf("upval %s\n", name);
      end = lrdb_write_value2(L, end, name, 1);
      lua_pop(L, 1);
    }
  } while(name != NULL);

  lua_pop(L, 1);

  end = json_objClose(end);
  end = json_objClose(end);
  end = json_end(end);
  end[0] = '\n';

  send(lrdb_data->fd, payload, (end - payload) + 1, 0);
}

void lrdb_get_globals(lua_State *L, LRDB_Data* lrdb_data, LRDB_Command* lrdb_command) {
  char payload[16384] = {0};
  char* end = json_objOpen(payload, NULL);
  
  end = json_str(end, "jsonrpc", "2.0");
  end = json_ulong(end, "id", lrdb_command->id);

  lua_pushinteger (L, LUA_RIDX_GLOBALS);
  lua_rawget(L, LUA_REGISTRYINDEX);

  end = lrdb_write_value2(L, end, "result", 2);
  end = json_objClose(end);
  end = json_end(end);
  end[0] = '\n';

  send(lrdb_data->fd, payload, (end - payload) + 1, 0);

  lua_pop(L, 1);
}

static inline void lrdb_set_nonblocking(int socket) {
  int flags=fcntl(socket, F_GETFL, 0);
  fcntl(socket, F_SETFL, flags | O_NONBLOCK);
}

static inline void lrdb_set_blocking(int socket) {
  int flags=fcntl(socket, F_GETFL, 0);
  fcntl(socket, F_SETFL, flags & ~O_NONBLOCK);
}

void lrdb_continue(LRDB_Data* lrdb_data, LRDB_Command* command) {
  // Set socket as non-blocking
  lrdb_set_nonblocking(lrdb_data->fd);

  // Set running
  lrdb_data->state = LRDB_STATE_RUNNING;

  // Return null as result
  lrdb_return_null(lrdb_data->fd, command->id);

  // Write running event
  lrdb_running(lrdb_data->fd);
}

void lrdb_pause(LRDB_Data* lrdb_data, LRDB_Command* command) {
  // Set socket as blocking
  lrdb_set_blocking(lrdb_data->fd);

  // Stop running
  lrdb_data->state = LRDB_STATE_STOPPED;

  // Return null as result
  lrdb_return_null(lrdb_data->fd, command->id);

  // Write running event
  lrdb_paused(lrdb_data->fd, "pause");
}

void lrdb_step(LRDB_Data* lrdb_data, LRDB_Command* command, int start_depth) {
  // Set socket as non-blocking
  lrdb_set_nonblocking(lrdb_data->fd);

  // Set to stepping
  lrdb_data->state = LRDB_STATE_STEPPING;
  lrdb_data->depth = start_depth;

  // Return null as result
  lrdb_return_null(lrdb_data->fd, command->id);

  // Write running event
  lrdb_running(lrdb_data->fd);
}

char* lrdb_write_value2(lua_State *L, char* json, const char* name, int recurse) {
  int table_index = lua_gettop(L);

  if(lua_isnil(L, -1)) {
    return json_null(json, name);
  } else if(lua_isboolean(L, -1)) {
    return json_bool(json, name, lua_toboolean(L, -1));
  } else if(lua_isinteger(L, -1)) {
    return json_ulong(json, name, lua_tointeger(L, -1));
  } else if(lua_isnumber(L, -1)) {
    double num = lua_tonumber(L, -1);
    if(isinf(num)) {
      return json_str(json, name, "Infinity");
    } else if(isnan(num)) {
      return json_str(json, name, "NaN");
    } else {
      return json_double(json, name, num);
    }
  } else if(lua_isstring(L, -1)) {
    return json_str(json, name, lua_tostring(L, -1));
  } else if(lua_istable(L, -1) && recurse > 0) {
    json = json_objOpen(json, name);

    lua_pushnil(L);
    while (lua_next(L, table_index) != 0) {
      // Duplicate key on stack
      lua_pushvalue(L, -2);

      // Perform string  conversion
      const char* key = lua_tostring(L, -1);
    
      // Pop duplicate from stack
      lua_pop(L, 1);

      // Serialize table
      lrdb_write_value2(L, json, key, recurse - 1);

      // Pop value from stack
      lua_pop(L, 1);
    }

    return json_objClose(json);
  } else {
    return json_str(json, name, lua_typename(L, lua_type(L, -1)));
  }
}

void lrdb_eval(lua_State *L, LRDB_Data* lrdb_data, LRDB_Command* command) {
  char payload[4096] = {0};
  char* end = payload;

  char chunk_return[strlen(command->chunk) + 8];
  int n1 = lua_gettop(L);
  
  // Try adding return in front first
  sprintf(chunk_return, "return %s", command->chunk);
  int result = luaL_loadstring (L, chunk_return);
  
  if(result != LUA_OK) {
    // Load the chunk as-is
    result = luaL_loadstring (L, command->chunk);
  }
  
  end = json_objOpen(end, NULL);
  end = json_int(end, "id", command->id);
  end = json_str(end, "jsonrpc", "2.0");
  end = json_arrOpen(end, "result");

  if(result == LUA_OK) {
    lua_pcall (L, 0, LUA_MULTRET, 0);
    int n2 = lua_gettop(L);
    for(int i = n1+1; i <= n2; i++) {
      lua_pushvalue(L, i);
      end = lrdb_write_value2(L, end, NULL, 0);
      lua_pop(L, 1);
    }
  } else {
    end = lrdb_write_value2(L, end, NULL, 0);
  }

  end = json_arrClose(end);
  end = json_objClose(end);
  end = json_end(end);

  end[0] = '\r';
  end[1] = '\n';

  send(lrdb_data->fd, payload, (end - payload) + 2, 0);
  
  lua_pop(L, 1);
}

void lrdb_clearbp(lua_State *L, LRDB_Data* lrdb_data, LRDB_Command* command) {
  // Set breakpoint count to 0
  lrdb_data->bp_count = 0;

  // Clear breakpoints table
  lua_newtable(L);
  lua_setfield(L, LUA_REGISTRYINDEX, "LRDB_BREAKPOINTS");

  // Return null as result
  lrdb_return_null(lrdb_data->fd, command->id);
}

void lrdb_addbp(lua_State *L, LRDB_Data* lrdb_data, LRDB_Command* command) {
  lrdb_data->bp_count++;

  printf("Add bp %s:%d [%d]\n", command->file, command->line, lrdb_data->bp_count);

  // Get table.insert() function from standard library
  lua_getglobal(L, "table");
  lua_getfield(L, -1, "insert");
  
  // Get breakpoints table
  lua_getfield(L, LUA_REGISTRYINDEX, "LRDB_BREAKPOINTS");

  if(lua_isnil(L, -1)) {
    // Remove nil
    lua_pop(L, 1);

    // Create new breakpoints table
    lua_newtable(L);
  }

  // Create new breakpoint
  lua_newtable(L);
  lua_pushstring(L, command->file);
  lua_setfield(L, -2, "file");
  lua_pushinteger(L, command->line);
  lua_setfield(L, -2, "line");

  lua_call(L, 2, 0); // Call

  // Return null as result
  lrdb_return_null(lrdb_data->fd, command->id);
}

LRDB_Command lrdb_parse_json(char* buffer) {
    LRDB_Command result = { .id = -1, .type = LRDB_COMMAND_INVALID, .chunk = NULL };
    const char* method;

    json_t pool[ MAX_FIELDS ];
    json_t const* json_obj;
    json_t const* params_obj;
    json_t const* parent = json_create( buffer, pool, MAX_FIELDS );

    if ( parent == NULL ) return result; // Not a json object

    // Check JSON RPC field
    json_obj = json_getProperty( parent, "jsonrpc" );
    if (json_obj == NULL) {
      return result; // No JSON RPC field
    }

    if (json_getType( json_obj ) != JSON_TEXT) {
      return result; // JSON RPC field is not a string
    }

    if(strcmp(json_getValue(json_obj), "2.0") != 0) {
      return result; // Invalid JSON RPC version
    }

    json_obj = json_getProperty( parent, "id" );
    if (json_getType(json_obj) != JSON_INTEGER) {
      return result; // id is not an integer
    }
    result.id = json_getInteger(json_obj);

    json_obj = json_getProperty( parent, "method" );
    if (json_obj == NULL) {
      return result; // No method field
    }

    if (json_getType( json_obj ) != JSON_TEXT) {
      return result; // method is not a string
    }

    method = json_getValue( json_obj );

    printf("id: %d, method: %s\n", result.id, method);

    if(strcmp("get_stacktrace", method) == 0) {
        result.type = LRDB_COMMAND_GETSTACKTRACE;
    } else if(strcmp("get_local_variable", method) == 0) {
        result.type = LRDB_COMMAND_GETLOCALVARIABLES;
    } else if(strcmp("get_upvalues", method) == 0) {
      result.type = LRDB_COMMAND_GETUPVALUES;
    } else if(strcmp("get_global", method) == 0) {
      result.type = LRDB_COMMAND_GETGLOBALS;
    } else if(strcmp("continue", method) == 0) {
      result.type = LRDB_COMMAND_CONTINUE;
    } else if(strcmp("step", method) == 0) {
      result.type = LRDB_COMMAND_STEP;
    } else if(strcmp("step_in", method) == 0) {
      result.type = LRDB_COMMAND_STEP_IN;
    } else if(strcmp("step_out", method) == 0) {
      result.type = LRDB_COMMAND_STEP_OUT;
    } else if(strcmp("eval", method) == 0) {
      json_obj = json_getProperty( parent, "params" );

      if (json_obj == NULL) {
        return result; // No params field
      }

      if (json_getType( json_obj ) != JSON_OBJ) {
        return result; // params is not an object
      }

      json_obj = json_getProperty( json_obj, "chunk" );

      if (json_obj == NULL) {
        return result; // No chunk field
      }

      if (json_getType( json_obj ) != JSON_TEXT) {
        return result; // chunk is not a string
      }

      const char* chunk = json_getValue(json_obj);
      result.chunk = (char*)malloc(strlen(chunk) + 1);
      strcpy(result.chunk, chunk);
      result.type = LRDB_COMMAND_EVAL;
    } else if(strcmp("clear_breakpoints", method) == 0) {
      result.type = LRDB_COMMAND_CLEARBP;
    } else if(strcmp("add_breakpoint", method) == 0) {
      result.type = LRDB_COMMAND_ADDBP;
      params_obj = json_getProperty( parent, "params" );

      if (params_obj == NULL) {
        return result; // No params field
      }

      if (json_getType( params_obj ) != JSON_OBJ) {
        return result; // params is not an object
      }

      json_obj = json_getProperty( params_obj, "line" );

      if (json_obj == NULL) {
        return result; // No line field
      }

      if (json_getType( json_obj ) != JSON_INTEGER) {
        return result; // line is not an int
      }

      result.line = json_getInteger(json_obj);

      json_obj = json_getProperty( params_obj, "file" );

      if (json_obj == NULL) {
        return result; // No file field
      }

      if (json_getType( json_obj ) != JSON_TEXT) {
        return result; // file is not a string
      }
      
      const char *file = json_getValue(json_obj);
      result.file = malloc(strlen(file) + 1);
      memcpy(result.file, file, strlen(file) + 1);

      result.type = LRDB_COMMAND_ADDBP;
    } else if(strcmp("pause", method) == 0) {
      result.type = LRDB_COMMAND_PAUSE;
    } else {
      printf("Unsupported command: %s\n", method);
    }

    return result;
}

void lrdb_socket_accept(LRDB_Data* lrdb_data) {
  struct sockaddr address;
  socklen_t addrlen;
  int accept_retry = 7;

  do {
    lrdb_data->fd = accept(lrdb_data->server_fd, &address, &addrlen);

    if(lrdb_data->fd < 0) {
      perror("accept");
      sleep(10);
    }
  } while(lrdb_data->fd < 0 && accept_retry > 0);

  if(lrdb_data->fd > 0) {
    lrdb_set_blocking(lrdb_data->fd);
    lrdb_connected(lrdb_data->fd);
    lrdb_paused(lrdb_data->fd, "entry");
    lrdb_data->state = LRDB_STATE_STOPPED;
  }
}

void lrdb_hook(lua_State *L, lua_Debug *ar) {
  lua_getstack (L, 0, ar);
  lua_getinfo(L, "S", ar);

  lua_getfield (L, LUA_REGISTRYINDEX, "LRDB_DATA");
  LRDB_Data* lrdb_data = lua_touserdata(L, -1);
  lua_pop(L, 1);

  if(lrdb_data->state == LRDB_STATE_STEPPING) {
    if(ar->event == LUA_HOOKCALL) {
      lrdb_data->depth++;
    } else if(ar->event == LUA_HOOKRET) {
      lrdb_data->depth--;
    } else if(ar->event == LUA_HOOKLINE && lrdb_data->depth <= 0) {
      lrdb_data->state = LRDB_STATE_STOPPED;
      lrdb_paused(lrdb_data->fd, "step");
      lrdb_set_blocking(lrdb_data->fd);
    }
  }

  if(lrdb_data->bp_count > 0 && ar->source[0] == '@' && lrdb_data->state != LRDB_STATE_STOPPED) {
    lua_getinfo(L, "l", ar);
    
    // Get breakpoints table
    lua_getfield(L, LUA_REGISTRYINDEX, "LRDB_BREAKPOINTS");

    int t = lua_gettop(L);

    lua_pushnil(L);
    while (lua_next(L, t) != 0) {
      lua_getfield(L, -1, "line");
      lua_getfield(L, -2, "file");

      if(lua_tointeger(L, -2) == ar->currentline && strcmp(lua_tostring(L, -1), &ar->source[1]) == 0) {
        lrdb_data->state = LRDB_STATE_STOPPED;
        lrdb_paused(lrdb_data->fd, "breakpoint");
        lrdb_set_blocking(lrdb_data->fd);
      }

      lua_pop(L, 3);
    }

    lua_pop(L, 1);
  }

  if(lrdb_data->fd > 0) {
    char buffer[1024] = {0};
    char* bufptr, *next;
    int length = 0;
    
    do {
      length = recv(lrdb_data->fd , buffer, 1024, 0);
      bufptr = buffer;
      if(length > 0) {
        do {
          next = strchr(bufptr, '\n');
          LRDB_Command command = lrdb_parse_json(bufptr);

          if(command.type == LRDB_COMMAND_GETSTACKTRACE) {
            lrdb_get_stack_trace(L, lrdb_data, &command);
          } else if(command.type == LRDB_COMMAND_GETLOCALVARIABLES) {
            lrdb_get_local_variables(L, lrdb_data, &command);
          } else if(command.type == LRDB_COMMAND_GETUPVALUES) {
            lrdb_get_upvalues(L, lrdb_data, &command);
          } else if(command.type == LRDB_COMMAND_GETGLOBALS) {
            lrdb_get_globals(L, lrdb_data, &command);
          } else if(command.type == LRDB_COMMAND_CONTINUE) {
            lrdb_continue(lrdb_data, &command);
          } else if(command.type == LRDB_COMMAND_PAUSE) {
            lrdb_pause(lrdb_data, &command);
          } else if(command.type == LRDB_COMMAND_STEP) {
            lrdb_step(lrdb_data, &command, 0);
          } else if(command.type == LRDB_COMMAND_STEP_IN) {
            lrdb_step(lrdb_data, &command, -1);
          } else if(command.type == LRDB_COMMAND_STEP_OUT) {
            lrdb_step(lrdb_data, &command, 1);
          } else if(command.type == LRDB_COMMAND_EVAL) {
            lrdb_eval(L, lrdb_data, &command);
          }else if(command.type == LRDB_COMMAND_CLEARBP) {
            lrdb_clearbp(L, lrdb_data, &command);
          } else if(command.type == LRDB_COMMAND_ADDBP) {
            lrdb_addbp(L, lrdb_data, &command);
          }

          if(command.chunk != NULL) {
            free(command.chunk);
          }

          if(command.file != NULL) {
            free(command.file);
          }

          bufptr = next + 1;
        } while(next != NULL);
      }

      if(length == -1 && errno != EAGAIN) {
        printf("errorno: %d\n", errno);
      }
    } while(lrdb_data->state == LRDB_STATE_STOPPED && length != -1);
  }
}

int lrdb_create_server(int* server_fd) {
  static u32 *SOC_buffer = NULL;
  struct sockaddr_in address;
  int bind_retry = 7, bind_success = 0, bind_result = 0, listen_result = 0, ret = 0;
  
  // allocate buffer for SOC service
  SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
  
  if(SOC_buffer == NULL) {
    printf("memalign: failed to allocate\n");
    return FAILURE;
  }

  // Now intialise soc:u service
  if ((ret = socInit(SOC_buffer, SOC_BUFFERSIZE)) != 0) {
    printf("socInit: 0x%08X\n", (unsigned int)ret);
    return FAILURE;
  }

  if ((*server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) == 0) { 
    perror("socket failed"); 
    return FAILURE; 
  }

  // setsockopt(*server_fd,SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option));

  address.sin_family = AF_INET; 
  address.sin_addr.s_addr = gethostid();
  address.sin_port = htons( LRDB_PORT );

  do {
    bind_result = bind(*server_fd, (struct sockaddr *)&address,  sizeof(address));
    if (bind_result < 0) { 
        perror("bind failed");
        sleep(10);
    } else {
      bind_success = 1;
    }
  } while(!bind_success && bind_retry > 0);

  if(!bind_success) {
    return FAILURE;
  }
  
  listen_result = listen(*server_fd, 3);
  if (listen_result < 0) { 
      perror("listen failed"); 
      return FAILURE; 
  }

  printf("Listening on %s:%u\n",inet_ntoa(address.sin_addr), LRDB_PORT);
  return SUCCESS;
}

void lrdb_register(lua_State *L) {
    LRDB_Data* lrdb_data = lua_newuserdata(L, sizeof(LRDB_Data));
    lrdb_data->server_fd = 0;
    lrdb_data->fd = 0;
    lrdb_data->state = LRDB_STATE_ENTRY;
    lrdb_data->depth = 0;
    
    if(lrdb_create_server(&lrdb_data->server_fd) == FAILURE) {
      printf("Could not create server\n");
      return;
    }

    lrdb_socket_accept(lrdb_data);
  
    lua_setfield (L, LUA_REGISTRYINDEX, "LRDB_DATA");
    lua_sethook(L, lrdb_hook, LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE, 0);
}

void lrdb_close(lua_State *L) {
  // Disable hook
  lua_sethook (L, lrdb_hook, 0, 0);

  // Get Data structure
  lua_getfield (L, LUA_REGISTRYINDEX, "LRDB_DATA");
  LRDB_Data* lrdb_data = lua_touserdata(L, -1);

  shutdown(lrdb_data->fd, SHUT_RDWR);
  shutdown(lrdb_data->server_fd, SHUT_RDWR);

  // Close sockets
  close(lrdb_data->fd);
  close(lrdb_data->server_fd);

  socExit();

  lua_pop(L, 1);
}

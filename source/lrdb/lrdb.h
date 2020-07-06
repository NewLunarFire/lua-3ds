#ifndef __LRDB_H__
#define __LRDB_H__

typedef enum LRBD_State {
  LRDB_STATE_STOPPED,
  LRDB_STATE_RUNNING,
  LRDB_STATE_STEPPING,
  LRDB_STATE_ENTRY
} LRBD_State;

typedef struct LRDB_Data {
  int server_fd;
  int fd;
  LRBD_State state;
  int depth;
  int bp_count;
} LRDB_Data;

typedef enum LRDB_CommandType {
    LRDB_COMMAND_INVALID,
    LRDB_COMMAND_GETSTACKTRACE,
    LRDB_COMMAND_GETLOCALVARIABLES,
    LRDB_COMMAND_GETUPVALUES,
    LRDB_COMMAND_GETGLOBALS,
    LRDB_COMMAND_CONTINUE,
    LRDB_COMMAND_PAUSE,
    LRDB_COMMAND_STEP,
    LRDB_COMMAND_STEP_IN,
    LRDB_COMMAND_STEP_OUT,
    LRDB_COMMAND_EVAL,
    LRDB_COMMAND_CLEARBP,
    LRDB_COMMAND_ADDBP
} LRDB_CommandType;

typedef struct LRDB_Command {
  int id;
  LRDB_CommandType type;
  char* chunk;
  char* file;
  int line;
} LRDB_Command;


void lrdb_register(lua_State *L);
void lrdb_close(lua_State *L);

#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <3ds.h>
#include <citro2d.h>
#include <citro3d.h>

#include "lua3.h"

int main() {
    // Initialize 3DS services
	gfxInitDefault();
	
    // Initialise configuration service
    cfguInit();
    
	// Init console for text output
	// consoleInit(GFX_BOTTOM, NULL);
    
    // Initialize C2D
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();

    Result rc = romfsInit();
    if (rc) printf("romfsInit: %08lX\n", rc);
    else lua3_runfile("romfs:/file-manager.lua");
    
    // Exit services
    cfguExit();
    gfxExit();

    return 0;
}
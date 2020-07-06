Lua 3DS
=======

This is a port of Lua 5.3.5 for 3DS with interfaces for the most common 3DS libraries

## Controls
A: Enter Directory / Run Script
B: Move up one directory
Y: Change Option
Start: Exit to HBL

## Debugger
The program currently has basic support for debugging with LRDB. To enable it, change the debugger option to lrdb before launching the script.
LRDB is supported in VSCode with the following extension: https://marketplace.visualstudio.com/items?itemName=satoren.lrdb

## Notes about authoring Scripts
You can use the standard Lua libraries to run script, as well as the following modules which have been preloaded. It is reccomended you take a
look at the lua/ directory for exemples and demonstrations on how to use Lua with the 3DS libraries. This program does not come with any file
transfer utilities built-in, so you need to either copy your scripts to the SD card or use another program like ftpd (https://github.com/mtheall/ftpd) to do so.

## Modules
### Citro2D

Those are the function for drawing 2D graphics

#### drawImage(image, x, y, scale, depth, tint, angle)
Draws an image at the screen at coordinates (x, y) with the provided scale and depth. Applies a tint if provided.

#### drawText(text, x, y, scale, depth, color)
Draws text at coordinates (x, y) with the provided scale, depth and color

#### drawRect(x, y, width, height, color)
Draws a solid rectangle at coordinates (x, y) with the provided color

#### drawRect(x, y, width, height, color1, color2, color3, color4)
Draws a rectangle at coordinates (x, y) with the specified dimensions and colors.

The colors are for each corner of the rectangle: color1: top-left, color2: top-right, color3: bottom-left, color4: bottom-right

#### drawTriangle(x1, y1, x2, y2, x3, y3, color[, depth])
Draws a triangle with corners at coordinates (x1, y1), (x2, y2), (x3, y3) with the specified color and depth

#### drawTriangle(x1, y1, x2, y2, x3, y3, color1, color2, color3, [, depth])
Draws a triangle with corners at coordinates (x1, y1), (x2, y2), (x3, y3) with the depth and a specified color for each corner

#### drawCircle(x, y, radius, color[, depth])
Draws a circle or radius r at coordinates (x, y) with the specified color and depth

#### drawCircle(x, y, radius, color1, color2, color3, color4, [, depth])
Draws a circle or radius r at coordinates (x, y) with the depth and a specified color for each corner

#### drawEllipse(x, y, width, height, color[, depth])
Draws an ellipse or width w and height h at coordinates (x, y) with the specified color and depth

#### drawEllipse(x, y, width, height, color1, color2, color3, color4, [, depth])
Draws an ellipse or radius r at coordinates (x, y) with the depth and a specified color for each corner

#### loadSpriteSheet(path)
Loads and returns the sprite sheet with the specified path

#### spriteSheetGetImage(sheet, index)
Returns the image at the specified index in the spritesheet. Index starts at 1.

#### parseColor(red, green, blue, alpha)
Takes the 4 arguments in the range 0-255 and transforms them to a u32 value representing this color

#### parseColor(table)
Takes a table with the r, g, b, and a fields in the range 0-255 and transforms it to a u32 value representing this color

### Game

This module exposes functions to allow the use of a game loop in the program. 

#### loop()
Starts a rendering loop that runs as long as exit() is not called. It continuously calls the global function onRender if it is defined.
This should be at the end of your main file, before cleanup code if necessary. 

#### exit()
Requests the exit of the game loop. The loop will complete and do the necessary cleanup once the current call of the onRender function returns.

#### cancel()
Cancels a previous exit request. This allows you to call the loop function again to restart the rendering loop.


### HID

This module exposes the controls of the console.

#### keysdown()
Returns the keys that have been pressed down since the last HID scan. If you use the game loop, the keys are scanned every loop iteration.

#### keysheld()
Returns the keys that are currently being held.

### CFGU

This modules allows you to get information on the current system configuration

#### getRegion([asString = true])
Returns the console region, as a string if asString is true, or as an integer code otherwise

#### getLanguage([asString = true])
Returns the console langugage, as a string if asString is true, or as an integer code otherwise

#### getModel([asString = true])
Returns the console model, as a string if asString is true, or as an integer code otherwise

#### isNFCSupported()
Returns if NFC is supported on this console
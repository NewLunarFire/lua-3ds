local C2D = require("citro2d")
local hid = require("hid")
local game = require("game")

local cur_step = 1
local step_texts = {"Load spritesheet"}

local sheet = C2D.loadSpriteSheet("romfs:/tex.t3x")
local logo = C2D.spriteSheetGetImage(sheet, 1)
local sphere = C2D.spriteSheetGetImage(sheet, 2)

local colors = {
    red = {r = 255, g = 0, b = 0},
    green = {r = 0, g = 255, b = 0},
    blue = {r = 0, g = 0, b = 255},
    yellow = {r = 255, g = 0, b = 255}
}

local tint = C2D.createTint(C2D.parseColor(255, 165, 0, 255), 0.5)

function onRender()
    C2D.drawText("Triangle", 0, 0, 0.5)
    C2D.drawTriangle(25, 75, 75, 75, 50, 25, colors.red, colors.green, colors.blue)

    C2D.drawText("Rectangle", 100, 0, 0.5)
    C2D.drawRect(125, 20, 50, 40, colors.red, colors.blue, colors.green, colors.yellow)

    C2D.drawText("Circle", 200, 0, 0.5)
    C2D.drawCircle(250, 45, 25, colors.red, colors.blue, colors.green, colors.yellow)

    C2D.drawText("Ellipse", 300, 0, 0.5)
    C2D.drawEllipse(325, 20, 50, 40, colors.red, colors.blue, colors.green, colors.yellow)

    C2D.drawText("Image", 0, 150, 0.5)
    C2D.drawImage(logo, 0, 150, 0.35, 1, tint, math.pi) 

    C2D.chooseScreen("bottom")
    C2D.drawText("Lua Citro2D Test", 0, 0, 0.5)
    C2D.drawText("Press START to exit", 0, 20, 0.5)

    local kdown = hid.keysdown()

    if (kdown & hid.keys.A) ~= 0 then
        step = step + 1
    end

    if (kdown & hid.keys.START) ~= 0 then
        game.exit()
    end

end

print("Press START to exit")
game.loop()
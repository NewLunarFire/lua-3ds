local C2D = require("citro2d")
local hid = require("hid")
local game = require("game")

local STEP_SIZE = 1/15
local red = C2D.parseColor(255, 0, 0, 255)
local white = C2D.parseColor(255, 255, 255, 255)

local direction = "RIGHT"
local snake = { {x = 20, y = 12} }
local fruit = {x = math.random(0, 39), y = math.random(0, 24)}
local last_clock = os.clock()
local game_over = false

function step()
    local x = snake[#snake].x
    local y = snake[#snake].y

    -- The wrap-around should give a game over instead
    if direction == "LEFT" then
        x = x -1
        if x < 0 then
            game_over = true
        end
    end
    if direction == "RIGHT" then
        x = x + 1
        if x > 39 then
            game_over = true
        end
    end
    if direction == "UP" then
        y = y -1
        if y < 0 then
            game_over = true
        end
    end
    if direction == "DOWN" then
        y = y + 1
        if y > 23 then
            game_over = true
        end
    end

    if snake[#snake].x == fruit.x and snake[#snake].y == fruit.y then
        -- It could pop back on top of the snake, it shouldn't
        fruit = {x = math.random(0, 39), y = math.random(0, 24)}
    else
        table.remove(snake, 1)
    end

    table.insert(snake, { x = x, y = y })
end

function onRender ()
    while math.floor((os.clock() - last_clock) / STEP_SIZE) >= 1 do
        step()
        last_clock = last_clock + STEP_SIZE
    end

    local kdown = hid.keysdown()

    for i = 1, #snake do
        C2D.drawRect(snake[i].x*10, snake[i].y*10, 10, 10, white)
    end
    
    C2D.drawRect(fruit.x*10, fruit.y*10, 10, 10, red)

    if (kdown & KEY_UP) ~= 0 then
        if direction ~= "DOWN" then
            direction = "UP"
        end
    end

    if (kdown & KEY_DOWN) ~= 0 then
        if direction ~= "UP" then
            direction = "DOWN"
        end
    end

    if (kdown & KEY_LEFT) ~= 0 then
        if direction ~= "RIGHT" then
            direction = "LEFT"
        end
    end

    if (kdown & KEY_RIGHT) ~= 0 then
        if direction ~= "LEFT" then
            direction = "RIGHT"
        end
    end

    if (kdown & KEY_START) ~= 0 then
        game.exit()
    end
end

print("Press START to quit")
game.loop()

hid = require("hid")

function onRender ()
    local keysdown = hid.keysdown()

    if (keysdown & hid.keys.START) ~= 0 then
        gameExit()
    end
end


print("Press START to quit")
gameLoop()

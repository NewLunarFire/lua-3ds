hid = require("hid")
C2D = require("citro2d")
game = require("game")

local PX_PER_LINE = 15
local PAGE_SIZE = 14
local SCREEN_WIDTH = 400
local SCREEN_HEIGHT = 240

local archive = "sdmc"
local cwd = { "lua" }
local debugger = "none"

setmetatable(cwd, {
    ["__tostring"] = function (dir)
        local s = "/"
        for i = 1, #dir do
            s = s .. dir[i] .. "/"
        end
        return s
    end,
    ["__concat"] = function (a, b)
        local s = a .. "/"
        for i = 1, #b do
            s = s .. b[i] .. "/"
        end
        return s
    end
})

local cursor = 1
local option_cursor = 1
local reload = true
local script = nil
-- local lua_logo = C2D.loadImage("romfs:/tex.t3x")
local logo_x = math.floor((SCREEN_WIDTH - 131) / 2)
local logo_y = math.floor((SCREEN_HEIGHT - 131) / 2)

local status_names = { [0] = "LUA_OK", [1] = "LUA_YIELD", [2] = "LUA_ERRRUN", [3] = "LUA_ERRSYNTAX", [4] = "LUA_ERRMEM", [5] = "LUA_ERRGCMM", [6] = "LUA_ERRERR" }
local status = 0
local message = nil

function listDirectoryFiles(cwd)
    local files = listDirectory(tostring(cwd))
    table.sort(files, function(a, b)
        if a.attributes.directory and not b.attributes.directory then
            return true
        elseif b.attributes.directory and not a.attributes.directory then
            return false
        else
            return a.name:upper() < b.name:upper()
        end
    end)
    return files
end

local files = listDirectoryFiles(cwd)

function drawEntry(entry, y)
    local attributes = ""
    if entry.attributes.directory then
        attributes = attributes .. "d"
    else
        attributes = attributes .. "-"
    end

    if entry.attributes.hidden  then
        attributes = attributes .. "h"
    else
        attributes = attributes .. "-"
    end

    if entry.attributes.archive then
        attributes = attributes .. "a"
    else
        attributes = attributes .. "-"
    end

    if entry.attributes.readonly then
        attributes = attributes .. "r"
    else
        attributes = attributes .. "-"
    end

    attributes = attributes .. " " .. entry.size

    C2D.drawText(entry.name, 20, y, 0.5)
    C2D.drawText(attributes, 300, y, 0.5)
end

function onRenderBottom(kdown)

end

function renderFileList(kdown)
    local page = math.ceil(cursor / PAGE_SIZE)
    local pcursor = cursor % PAGE_SIZE
    if pcursor == 0 then
        pcursor = 14
    end
    local start = ((page - 1) * 14) + 1
    local stop = start + 13
    if stop > #files then
        stop = #files
    end
    

    -- C2D.drawImage(lua_logo, logo_x, logo_y, 1.0, -1.0)
    C2D.drawText("Directory: " .. archive .. ":" .. cwd, 0, 0, 0.5)
    C2D.drawText("Page " .. page .. "/" .. math.ceil(#files / PAGE_SIZE), 300, 0, 0.5) 

    local status_message = status_names[status]
    if message ~= nil then
        status_message = status_message .. ": " .. message
    end
    C2D.drawText("Status: " .. status_message, 0, 225, 0.5)

    C2D.drawText("->", 0, (PX_PER_LINE*pcursor), 0.5)

    for i = start, stop do
        drawEntry(files[i], PX_PER_LINE*(i + 1 - start))
    end

    if (kdown & hid.keys.A) ~= 0 then
        if files[cursor].attributes.directory then
            table.insert(cwd, files[cursor].name)
            files = listDirectoryFiles(cwd)
            cursor = 1
        else
            script = files[cursor].name
            game.exit()
        end
        
    end

    if (kdown & hid.keys.B) ~= 0 then
        table.remove(cwd)
        files = listDirectoryFiles(cwd)
        cursor = 1
    end

    if (kdown & hid.keys.UP) ~= 0 then
        cursor = cursor - 1
        if cursor < 1 then
            cursor = 1
        end
    end
    
    if (kdown & hid.keys.DOWN) ~= 0 then
        cursor = cursor + 1
        if cursor > #files then
            cursor = #files
        end
    end
end

function renderOptions (kdown)
    C2D.drawText("Options", 0, 0, 0.5)
    C2D.drawText("->", 0, PX_PER_LINE, 0.5)
    C2D.drawText("Debugger: " .. debugger, 20, PX_PER_LINE, 0.5)

    if (kdown & hid.keys.Y) ~= 0 then
        if debugger == "none" then
            debugger = "lrdb"
        else
            debugger = "none"
        end
    end
end

function onRender ()
    local kdown = hid.keysdown()
    
    renderFileList(kdown)
    
    C2D.chooseScreen("bottom")
    renderOptions(kdown)

    if (kdown & hid.keys.START) ~= 0 then
        reload = false
        game.exit()
    end
end


while reload do
    game.cancel()
    game.loop()

    if script then
        status, message = runScript(archive .. ":" .. cwd, script, debugger)
        script = nil
    end
end

C2D = require("citro2d")
hid = require("hid")
game = require("game")
cfgu = require("cfgu")

region = cfgu.getRegion()
region_code = cfgu.getRegion(false)
language = cfgu.getLanguage()
language_code = cfgu.getLanguage(false)
model = cfgu.getModel()
model_code = cfgu.getModel(false)
nfc_supported = cfgu.isNFCSupported()

function onRender ()
    C2D.drawText("CFGU Test", 0, 0, 0.5)
    C2D.drawText("Region: " .. region .. " [" .. region_code .. "]", 0, 15, 0.5)
    C2D.drawText("Language: " .. language .. " [" .. language_code .. "]", 0, 30, 0.5)
    C2D.drawText("Model: " .. model .. " [" .. model_code .. "]", 0, 45, 0.5)

    if nfc_supported then
        C2D.drawText("NFC is supported", 0, 60, 0.5)
    else
        C2D.drawText("NFC is not supported", 0, 60, 0.5)
    end

    C2D.drawText("Press START to exit", 0, 225, 0.5)

    local kdown = hid.keysdown()
    if (kdown & hid.keys.START) ~= 0 then
        game.exit()
    end
end


game.loop()
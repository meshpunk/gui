--[[
  Grid based launcher for MeshPunks
]]

local lvgl = require("lvgl")
local toml = require("lib/toml")

-- Constants for grid layout
local GRID_COLS = 4
local GRID_ROWS = 3
local ICON_SIZE = 64
local PADDING = 10

local function create_launcher(parent)
    local root = lvgl.Object({
        flex = {
            flex_direction = "row",
            flex_wrap = "wrap",
            justify_content = "center",
            align_items = "center",
            align_content = "center",
        },
        w = 300,
        h = 75,
        align = lvgl.ALIGN.CENTER,
    })

    for i = 1, 10 do
        local item = root:Object({
            w = 100,
            h = lvgl.PCT(100),
        })
        item:clear_flag(lvgl.FLAG.SCROLLABLE)

        local label = item:Label({
            text = string.format("label %d", i),
        })
        label:center()
    end

    -- Load apps from TOML
    local file = io.open("/lua/apps.toml", "r")
    if not file then error("where is apps.toml?") end
    local content = file:read()
    file:close()
    print(content)

    local apps = toml.parse(content)

    -- Create app buttons
    local index = 0
    for name, app in pairs(apps) do
        local item = root:Object({
            w = 100,
            h = lvgl.PCT(100),
        })
        item:clear_flag(lvgl.FLAG.SCROLLABLE)
    end

    return root
end

return {
    create = create_launcher,
}

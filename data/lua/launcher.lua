--[[
  Grid based launcher for MeshPunks
]]

local lvgl = require("lvgl")
local toml = require("lib/toml")
local messages = require("lib/mesh/messages")

-- Constants for grid layout
local GRID_COLS = 4
local GRID_ROWS = 3
local ICON_SIZE = 64
local PADDING = 10

local function create_launcher(parent)
    local unread = 0

    -- Get past messages
    for _, msg in ipairs(messages:all()) do
        unread = unread + 1
    end

    local root = lvgl.Object({
        flex = {
            flex_direction = "row",
            flex_wrap = "wrap",
            justify_content = "center",
            align_items = "center",
            align_content = "center",
        },
        w = 300,
        h = 240,
        align = lvgl.ALIGN.CENTER,
    })

    -- Load apps from TOML
    local file = io.open("/lua/apps.toml", "r")
    if not file then error("where is apps.toml?") end
    local content = file:read()
    file:close()
    print(content)

    -- Connect button
    local indicator = root:Label{text = unread .. ' unread', align = lvgl.ALIGN.CENTER, w = 100, h = 40}

    -- React to new messages
    messages:onMessage(function(msg)
        print("recv!")
    --   print(msg)
    --   print(msg.timestamp .. " 🕒")
    --   print(msg.hops .. "🐰")

      unread = unread + 1
      print(unread .. " unread")
      indicator.text = unread .. ' unread'
      print("set indicator again!")
    end)

    local config = toml.parse(content)

    -- Create app buttons
    local index = 0
    for _, app in ipairs(config.apps) do
      local name = app.name or "Unnamed"
      local entrypoint = "/lua" .. (app.entry or "main.lua")
      local icon = app.icon and ("/lua" .. app.icon) or "/lua/icon.png"

        print("Creating app button for", name, entrypoint)

        -- Connect button
        local btn = root:Button{w = 100, h = 40}
        btn:Label{text = name, align = lvgl.ALIGN.CENTER}

        btn:onClicked(function()
            print("Launching app:", entrypoint)

            -- Close launcher and launch app
            root:delete()

            -- Dofile...
            local success, err = pcall(function()
                dofile(entrypoint)
            end)
            if not success then
                print("Error launching app:", err)
            end
        end)
      end

    return root
end

return {
    create = create_launcher,
}

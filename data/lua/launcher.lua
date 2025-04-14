--[[
  Grid based launcher for MeshPunks
]]

local lvgl = require("lvgl")
local toml = require("toml")

-- Constants for grid layout
local GRID_COLS = 4
local GRID_ROWS = 3
local ICON_SIZE = 64
local PADDING = 10

local function create_launcher(parent)
    local container = lvgl.obj(parent)
    container:set_size("100%", "100%")
    
    -- Create a grid layout
    local grid = lvgl.grid_create(container)
    container:set_layout(lvgl.GRID_LAYOUT)
    
    -- Configure grid
    local col_dsc = {}
    for i = 1, GRID_COLS do
        table.insert(col_dsc, ICON_SIZE)
        table.insert(col_dsc, PADDING)
    end
    
    local row_dsc = {}
    for i = 1, GRID_ROWS do
        table.insert(row_dsc, ICON_SIZE + 20)  -- Extra space for label
        table.insert(row_dsc, PADDING)
    end
    
    container:set_grid_dsc_array(col_dsc, row_dsc)
    
    -- Load apps from TOML
    local file = io.open("apps.toml", "r")
    if not file then
        error("Could not find apps.toml")
    end
    local apps = toml.parse(file:read("*all"))
    file:close()
    
    -- Create app buttons
    local index = 0
    for name, app in pairs(apps) do
        if index < (GRID_COLS * GRID_ROWS) then
            local col = index % GRID_COLS
            local row = math.floor(index / GRID_COLS)
            
            -- Create button container
            local btn = lvgl.btn(container)
            btn:set_grid_cell(col * 2, 1, row * 2, 1)
            
            -- Create vertical layout for icon and label
            local layout = lvgl.flex_create(btn)
            btn:set_layout(lvgl.FLEX_LAYOUT)
            btn:set_flex_flow(lvgl.FLEX_FLOW.COLUMN)
            btn:set_flex_align(lvgl.FLEX_ALIGN.CENTER, lvgl.FLEX_ALIGN.CENTER, lvgl.FLEX_ALIGN.CENTER)
            
            -- Add icon if specified
            if app.icon then
                local img = lvgl.img(btn)
                img:set_src(app.icon)
                img:set_size(ICON_SIZE, ICON_SIZE)
            end
            
            -- Add label
            local label = lvgl.label(btn)
            label:set_text(name)
            
            -- Add click handler
            if app.command then
                btn:add_event_cb(function()
                    -- Execute app command
                    os.execute(app.command)
                end, lvgl.EVENT.CLICKED, nil)
            end
            
            index = index + 1
        end
    end
    
    return container
end

return {
    create = create_launcher
}
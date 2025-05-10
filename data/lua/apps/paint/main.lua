local ui_utils = require("ui-utils")

-- root object
local root = lvgl.Object()
root:set { 
    w = lvgl.HOR_RES(), 
    h = lvgl.VER_RES(),
    border_width = 0,
    radius = 0,
    pad_all = 0,
    bg_color = "#aaaaaa",
}:clear_flag(lvgl.FLAG.SCROLLABLE)

local COLORS = { "#211e20", "#555568", "#a0a08b", "#e9efec" }

local IMAGE_SIZE = 8
local PIXEL_SIZE = math.floor(lvgl.VER_RES() / (IMAGE_SIZE + 1))
local CANVAS_SIZE = PIXEL_SIZE * IMAGE_SIZE
local BORDER_SIZE = (lvgl.VER_RES() - CANVAS_SIZE) / 2
local SIDEBAR_SIZE = (lvgl.HOR_RES() - CANVAS_SIZE) / 2 - BORDER_SIZE * 2

local current_colour = 1

-- palette buttons

local palette = root:Object {
    w = SIDEBAR_SIZE,
    h = CANVAS_SIZE,
    x = BORDER_SIZE,
    y = BORDER_SIZE,
    bg_color = "#ffffff",
    outline_width = 1,
    outline_color = "#000000",
    radius = 0,
    border_width = 0,
    pad_all = 0,
    pad_gap = 0,
    flex = {
        flex_direction = "column",
        justify_content = "center",
        align_items = "center",
    }
}:clear_flag(lvgl.FLAG.SCROLLABLE)

for i, colour in ipairs(COLORS) do
    local btn = palette:Button {
        w = SIDEBAR_SIZE,
        h = CANVAS_SIZE / #COLORS,
        bg_color = colour,
        radius = 0,
        border_width = 0,
    }
    btn:onClicked(function()
        if current_colour == i then return end
        current_colour = i
        ui_utils.propagate_state(palette, lvgl.STATE.CHECKED, false)
        btn:add_state(lvgl.STATE.CHECKED)
    end)

    btn:add_style(lvgl.Style {
        border_color = "#ffffff",
        border_width = 1,
        bg_color = colour,
    }, lvgl.STATE.CHECKED)

    if i == current_colour then
        btn:add_state(lvgl.STATE.CHECKED)
    end
end

-- TinyImage preview

local tiny_img = require("apps/paint/tinyimage"):new{ width = IMAGE_SIZE, height = IMAGE_SIZE, palette = COLORS }

local tiny_preview = root:Object {
    w = IMAGE_SIZE * 4,
    h = IMAGE_SIZE * 4,
    x = lvgl.HOR_RES() - IMAGE_SIZE * 4 - BORDER_SIZE,
    y = BORDER_SIZE,
    outline_color = "#000000",
    outline_width = 1,
    bg_color = "#ffffff",
    radius = 0,
    border_width = 0,
    pad_all = 0,
}:clear_flag(lvgl.FLAG.SCROLLABLE)
tiny_img:draw(tiny_preview)

-- canvas for actually editing the image

local canvas = root:Object {
    w = CANVAS_SIZE,
    h = CANVAS_SIZE,
    bg_color = "#000000",
    outline_width = 1,
    outline_color = "#000000",
    radius = 0,
    border_width = 0,
    align = lvgl.ALIGN.CENTER,
    pad_all = 0,
}:clear_flag(lvgl.FLAG.SCROLLABLE)

for i = 1, IMAGE_SIZE do
    for j = 1, IMAGE_SIZE do
        local btn = canvas:Object {
            w = PIXEL_SIZE,
            h = PIXEL_SIZE,
            bg_color = COLORS[1],
            radius = 0,
            border_width = 0,
            x = (i - 1) * PIXEL_SIZE,
            y = (j - 1) * PIXEL_SIZE,
        }
        btn:add_flag(lvgl.FLAG.CLICKABLE)
        btn:clear_flag(lvgl.FLAG.SCROLLABLE)
        btn:onClicked(function() 
            btn.bg_color = COLORS[current_colour] 
            
            -- -- Update TinyImage
            tiny_img:set_pixel(i-1, j-1, current_colour - 1)  -- Convert to 0-based index
            -- Update TinyImage preview
            tiny_preview:get_child((j - 1) + (i - 1) * IMAGE_SIZE).bg_color = COLORS[current_colour]
        end)
    end
end
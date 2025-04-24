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

local dimension = 8
local PIXEL_SIZE = math.floor(lvgl.VER_RES() / (dimension + 1))
local CANVAS_SIZE = PIXEL_SIZE * dimension
local BORDER_SIZE = (lvgl.VER_RES() - CANVAS_SIZE) / 2
local SIDEBAR_SIZE = (lvgl.HOR_RES() - CANVAS_SIZE) / 2 - BORDER_SIZE * 2

local pixels = {}
local colours = { "#211e20", "#555568", "#a0a08b", "#e9efec"}
local current_colour = colours[1]

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

for i, colour in ipairs(colours) do
    local btn = palette:Button {
        w = SIDEBAR_SIZE,
        h = CANVAS_SIZE / #colours,
        bg_color = colour,
        radius = 0,
        border_width = 0,
    }
    btn:onClicked(function()
        current_colour = colour
    end)
end

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

for i = 1, dimension do
    for j = 1, dimension do
        local btn = canvas:Button {
            w = PIXEL_SIZE,
            h = PIXEL_SIZE,
            bg_color = colours[1],
            radius = 0,
            border_width = 0,
            x = (i - 1) * PIXEL_SIZE,
            y = (j - 1) * PIXEL_SIZE,
        }
        btn:clear_flag(lvgl.FLAG.SCROLLABLE)
        btn:onClicked(function() btn.bg_color = current_colour end)
    end
end
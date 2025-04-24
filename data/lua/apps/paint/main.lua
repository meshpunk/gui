

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

local pixels = {}

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
            bg_color = "#ffffff",
            radius = 0,
            border_width = 0,
            x = (i - 1) * PIXEL_SIZE,
            y = (j - 1) * PIXEL_SIZE,
        }
        btn:clear_flag(lvgl.FLAG.SCROLLABLE)
        btn:onClicked(function()
            print(btn.bg_color)
            btn.bg_color = (btn.bg_color == lvgl.color_to_int("#ffffff")) and "#000000" or "#ffffff"
        end)
    end
end
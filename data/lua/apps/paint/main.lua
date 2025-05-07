local ui_utils = require("ui-utils")
local tinyimage = require("apps/paint/tinyimage")

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

local image = require("apps/paint/image"):new {
    size = 8,
    palette = COLORS,
    data = string.rep("1", 8 * 8)
}

-- Create TinyImage version
local tiny_img = tinyimage:new({width = 8, height = 8, palette = COLORS})
-- -- Initialize with same data as regular image
-- for i = 1, 8 do
--     for j = 1, 8 do
--         local index = j + (i - 1) * 8
--         local color_index = tonumber(image.data:sub(index, index))
--         tiny_img:set_pixel(i-1, j-1, color_index - 1)  -- TinyImage uses 0-based indices
--     end
-- end

local PIXEL_SIZE = math.floor(lvgl.VER_RES() / (image.size + 1))
local CANVAS_SIZE = PIXEL_SIZE * image.size
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

-- previews in the corner

local preview = root:Object {
    w = image.size * 2,
    h = image.size * 2,
    x = lvgl.HOR_RES() - image.size * 2 - BORDER_SIZE * 2,
    y = BORDER_SIZE,
    outline_color = "#000000",
    outline_width = 1,
    bg_color = "#ffffff",
    radius = 0,
    border_width = 0,
    pad_all = 0,
}:clear_flag(lvgl.FLAG.SCROLLABLE)
image:draw(preview)

-- TinyImage preview
local tiny_preview = root:Object {
    w = image.size * 2,
    h = image.size * 2,
    x = lvgl.HOR_RES() - image.size * 2 - BORDER_SIZE,
    y = BORDER_SIZE * 3,
    outline_color = "#000000",
    outline_width = 1,
    bg_color = "#ffffff",
    radius = 0,
    border_width = 0,
    pad_all = 0,
}:clear_flag(lvgl.FLAG.SCROLLABLE)
tiny_img:draw(tiny_preview)
-- -- Draw TinyImage preview
-- for i = 0, 7 do
--     for j = 0, 7 do
--         local color_index = tiny_img:get_pixel(i, j)
--         local color = COLORS[color_index + 1]  -- Convert back to 1-based index
--         tiny_preview:Object {
--             w = 2,
--             h = 2,
--             x = i * 2,
--             y = j * 2,
--             bg_color = color,
--             radius = 0,
--             border_width = 0,
--         }
--     end
-- end

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

for i = 1, image.size do
    for j = 1, image.size do
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
            local index = j + (i - 1) * image.size
            if image.data[index] == COLORS[current_colour] then return end

            image.data = image.data:sub(1, index - 1) .. COLORS[current_colour] .. image.data:sub(index + 1)
            btn.bg_color = COLORS[current_colour] 

            -- Update both previews
            preview:get_child((j - 1) + (i - 1) * image.size).bg_color = COLORS[current_colour]
            
            -- -- Update TinyImage
            tiny_img:set_pixel(i-1, j-1, current_colour - 1)  -- Convert to 0-based index
            -- Update TinyImage preview
            tiny_preview:get_child((j - 1) + (i - 1) * image.size).bg_color = COLORS[current_colour]
            -- tiny_preview:clean()
            -- tiny_img:draw(tiny_preview)
        end)
    end
end
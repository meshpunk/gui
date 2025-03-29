
-- DATA
local clues = "4.....8.5.3..........7......2.....6.....8.4......1.......6.3.7.5..2.....1.4......"
local selected_cell = 1

-- Basic checks
-- function configuration_is_valid(board) 
--     if #board != 81 then return false end
-- end

-- RENDER
local root = lvgl.Object()
root:set { 
    w = lvgl.HOR_RES(), 
    h = lvgl.VER_RES(),
    border_width = 0,
    radius = 0,
    pad_all = 0,
    bg_color = "#aaaaaa"
}
root:clear_flag(lvgl.FLAG.SCROLLABLE)

-- find closest multiple of 9
local board_size = math.floor((lvgl.VER_RES()  - 20 ) / 9) * 9

local board = root:Object {
    w = board_size,
    h = board_size,
    radius = 0,
    outline_width = 1,
    outline_pad = 0,
    border_width = 0,
    outline_color = "#000000",
    bg_color = "#ffffff",
    align = lvgl.ALIGN.CENTER,
    pad_all = 0,
    pad_gap = 0,
    flex = {
        flex_direction = "column",
        justify_content = "center",
        align_items = "center",
    }
}
board:clear_flag(lvgl.FLAG.SCROLLABLE)

for i = 0, 2 do 
    local subrow = board:Object {
        w = board_size,
        h = math.floor(board_size / 3),
        bg_color = "#dddddd",
        border_width = 0,
        radius = 0,
        flex = {
            flex_direction = "row",
            justify_content = "center",
            align_items = "center",
        },
        pad_all = 0,
        pad_gap = 0,
    }
    subrow:clear_flag(lvgl.FLAG.SCROLLABLE)

    for j = 0, 2 do
        local subcol = subrow:Object {
            w = board_size / 3,
            h = board_size / 3,
            bg_color = "#ffffff",
            border_width = 1,
            border_color = "#000000",
            radius = 0,
            align = lvgl.ALIGN.CENTER,
            pad_all = 0,
            pad_gap = 0,
            flex = {
                flex_direction = "column",
                justify_content = "center",
                align_items = "center",
            }
        }
        subcol:clear_flag(lvgl.FLAG.SCROLLABLE)

        for k = 0, 2 do
            local row = subcol:Object {
                w = board_size / 3,
                h = board_size / 9,
                bg_color = "#ffffff",
                border_width = 0,
                radius = 0,
                flex = {
                    flex_direction = "row",
                    justify_content = "center",
                    align_items = "center",
                },
                pad_all = 0,
                pad_gap = 0,
            }
            row:clear_flag(lvgl.FLAG.SCROLLABLE)

            for l = 0, 2 do
                local cell_index = (i * 3 + k) * 9 + (j * 3 + l) + 1
                
                local cell = row:Object {
                    w = board_size / 9,
                    h = board_size / 9,
                    bg_color = "#ffffff",
                    border_width = 1,
                    radius = 0,
                }
                cell:clear_flag(lvgl.FLAG.SCROLLABLE)

                -- if cell_index == selected_cell then 
                --     cell:set_style("bg_color", "#000000")
                -- end
                
                local value = clues:sub(cell_index, cell_index)
                if value ~= "." then
                    local text = cell:Label {
                        -- text = "",
                        text = value,
                        align = lvgl.ALIGN.CENTER,
                        pad_all = 0,
                        pad_gap = 0,
                    }
                end
            end
        end
    end
end
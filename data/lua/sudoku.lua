-- DATA
local clues = "4.....8.5.3..........7......2.....6.....8.4......1.......6.3.7.5..2.....1.4......"
local selected = 41

local cells = {}
for i = 1, #clues do
    local clue = clues:sub(i, i)
    cells[i] = {
        value = clue,
        notes = {},
        mutable = clue == ".",
        object = nil,
        setSelected = function (self, selected)
            local fn = selected and "add_state" or "clear_state"

            self.object[fn](self.object, lvgl.STATE.CHECKED)
            for i = 0, self.object:get_child_cnt() - 1 do
                local child = self.object:get_child(i)
                child[fn](child, lvgl.STATE.CHECKED)
            end
        end,
    }
end
cells[2].notes[1] = true
cells[2].notes[5] = true
cells[2].notes[9] = true

cells[79].notes[2] = true
cells[79].notes[3] = true
cells[79].notes[7] = true

local capital_keys_map = {
    W = 1, E = 2, R = 3,
    S = 4, D = 5, F = 6,
    Z = 7, X = 8, C = 9,
}
local capital_keys = {}
for key, _ in pairs(capital_keys_map) do table.insert(capital_keys, key) end

-- local small_keys_map = {
--     w = 1, e = 2, r = 3,
--     s = 4, d = 5, f = 6,
--     z = 7, x = 8, c = 9,
-- }
-- local small_keys = {}
-- for key, _ in pairs(small_keys_map) do table.insert(small_keys, key) end

-- compute sizes
local cell_size = math.floor((lvgl.VER_RES()  - 20 ) / 9) -- closest multiple of 9 to desired size
local board_size = cell_size * 9
local note_marker_size = math.floor((cell_size - 4) / 3)
local third_size = math.floor((cell_size - 2) / 3)

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

local group = lvgl.group.create()
group:add_obj(board)
group:set_default()

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
                    w = cell_size,
                    h = cell_size,
                    bg_color = "#ffffff",
                    border_width = 1,
                    radius = 0,
                    pad_all = 0,
                }
                cell:clear_flag(lvgl.FLAG.SCROLLABLE)

                cell:add_style(lvgl.Style {
                    bg_color = "#9ad4e3",
                    border_color = "#588bb8",
                }, lvgl.STATE.CHECKED)
                cells[cell_index].object = cell

                local value = clues:sub(cell_index, cell_index)
                if value ~= "." then
                    local text_color = "#000000"
                    if not cells[cell_index].mutable then text_color = "#808080" end

                    local text = cell:Label {
                        text = value,
                        align = lvgl.ALIGN.CENTER,
                        text_color = text_color,
                        pad_all = 0,
                        pad_gap = 0,
                    }
                else
                    for m = 1, 9 do
                        if cells[cell_index].notes[m] then
                            local note = cell:Object {
                                w = note_marker_size,
                                h = note_marker_size,
                                bg_color = "#c4c4c4",
                                border_width = 1,
                                x = ((m - 1) % 3) * note_marker_size + 2,
                                y = math.floor((m - 1) / 3) * note_marker_size + 2,
                                radius = 0,
                            }
                            note:add_style(lvgl.Style {
                                bg_color = "#82b5c2",
                                border_color = "#588bb8",
                            }, lvgl.STATE.CHECKED)
                        end
                    end
                end
            end
        end
    end
end
cells[selected].object:add_state(lvgl.STATE.CHECKED)

-- Get the keyboard input device and connect it to our group
local keyboard = lvgl.indev.get_next()
if not keyboard then error("No keyboard input device found") end
keyboard:set_group(group)

-- Add keyboard event handler to button
board:onevent(lvgl.EVENT.KEY, function(obj, code)
    local indev = lvgl.indev.get_act()
    if not indev then return end

    local key = string.char(indev:get_key()) 
    local selected_cell = cells[selected]

    if key == "i" or key == "j" or key == "k" or key == "l" then
        selected_cell:setSelected(false)

        if key == "i" then
            selected = selected - 9
            if selected < 1 then selected = selected + 81 end
        elseif key == "j" then
            selected = selected - 1
            if selected % 9 == 0 then selected = selected + 9 end
        elseif key == "k" then
            selected = selected + 9
            if selected > 81 then selected = selected - 81 end
        elseif key == "l" then
            selected = selected + 1
            if selected % 9 == 1 then selected = selected - 9 end    
        end

        cells[selected]:setSelected(true)
        return
    elseif not selected_cell.mutable then return end

    local is_capital_number = false
    for _, capital_key in ipairs(capital_keys) do
        if key == capital_key then is_capital_number = true break end
    end
    if is_capital_number then
        local number = capital_keys_map[key]
        if selected_cell.notes[number] then
            selected_cell.notes[number] = false
        else
            selected_cell.notes[number] = true
        end
        print(selected_cell.notes[number])
    end
end)
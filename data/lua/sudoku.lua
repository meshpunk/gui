-- compute sizes
local cell_size = math.floor((lvgl.VER_RES()  - 20 ) / 9) -- closest multiple of 9 to desired size
local board_size = cell_size * 9
local note_marker_size = math.floor((cell_size - 4) / 3)
local third_size = math.floor((cell_size - 2) / 3)

-- DATA
local clues = ".....4.284.6.....51...3.6.....3.1....87...14....7.9.....2.1...39.....5.767.4....."
-- local clues = "7351649284269783151985326742493817563872561495617498328526174939148235676734952.."
assert(#clues == 81)

local remaining = 81
for i = 1, #clues do if clues:sub(i, i) ~= "." then remaining = remaining - 1 end end

local selected = 41

local cells = {
    coordinates = function (self, index)
        return {
            row = math.floor((index - 1) / 9) + 1,
            col = (index - 1) % 9 + 1
        }
    end,
    at = function (self, row, col)
        return self[(row - 1) * 9 + col]
    end,
    row = function (self, index)
        local row = {}
        for i = 1, 9 do table.insert(row, self:at(index, i)) end
        return row
    end,
    col = function (self, index)
        local col = {}
        for i = 1, 9 do table.insert(col, self:at(i, index)) end
        return col
    end,
    box = function (self, row, col)
        local box = {}
        row = math.floor((row - 1) / 3) * 3
        col = math.floor((col - 1) / 3) * 3
        for i = 1, 3 do
            for j = 1, 3 do
                table.insert(box, self:at(row + i, col + j))
            end
        end
        return box
    end,
}

local Cell = {
    text_color = {
        normal = "#000000",
        immutable = "#808080",
    },
    setSelected = function (self, selected)
        local fn = selected and "add_state" or "clear_state"

        self.object[fn](self.object, lvgl.STATE.EDITED)
        for i = 0, self.object:get_child_cnt() - 1 do -- propagate to children
            local child = self.object:get_child(i)
            child[fn](child, lvgl.STATE.EDITED)
        end

        -- set checked for all matching numbers
        if self.value ~= "." then for i, cell in ipairs(cells) do
            if cell.value == self.value then cell:setChecked(selected) end
        end end
    end,
    setChecked = function (self, checked)
        print("set checked")
        local fn = checked and "add_state" or "clear_state"
        self.object[fn](self.object, lvgl.STATE.CHECKED)
        for i = 0, self.object:get_child_cnt() - 1 do 
            local child = self.object:get_child(i)
            child[fn](child, lvgl.STATE.CHECKED)
        end
    end,
    render_children = function (self)
        self.object:clean()
        if self.value == "." then
            for m = 1, 9 do
                if self.notes[m] then
                    self.object:Object {
                        w = note_marker_size,
                        h = note_marker_size,
                        bg_color = "#c4c4c4",
                        border_width = 1,
                        x = ((m - 1) % 3) * note_marker_size + 2,
                        y = math.floor((m - 1) / 3) * note_marker_size + 2,
                        radius = 0,
                    }:add_style(lvgl.Style {
                        bg_color = "#82b5c2",
                        border_color = "#588bb8",
                    }, lvgl.STATE.EDITED)
                end
            end
        else 
            self.object:Label {
                text = self.value,
                align = lvgl.ALIGN.CENTER,
                text_color = self.text_color,
                pad_all = 0,
                pad_gap = 0,
            }:add_style(lvgl.Style {
                text_color = "#588bb8",
            }, lvgl.STATE.CHECKED)
        end
    end
}

function Cell:new(value)
    local o = {}
    o.notes = {}
    o.value = value
    o.mutable = value == "."
    o.text_color = (o.mutable and Cell.text_color.normal) or Cell.text_color.immutable

    setmetatable(o, self)
    self.__index = self
    return o
end

for i = 1, #clues do cells[i] = Cell:new(clues:sub(i, i)) end

-- keyboard numpad letters to numbers
local capital_keys_map = {
    W = 1, E = 2, R = 3,
    S = 4, D = 5, F = 6,
    Z = 7, X = 8, C = 9,
}
local capital_keys = {}
for key, _ in pairs(capital_keys_map) do table.insert(capital_keys, key) end

local small_keys_map = {
    w = 1, e = 2, r = 3,
    s = 4, d = 5, f = 6,
    z = 7, x = 8, c = 9,
}
local small_keys = {}
for key, _ in pairs(small_keys_map) do table.insert(small_keys, key) end

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
                    :add_style(lvgl.Style {
                        bg_color = "#9ad4e3",
                        border_color = "#588bb8",
                    }, lvgl.STATE.EDITED)
                    :clear_flag(lvgl.FLAG.SCROLLABLE)
                cells[cell_index].object = cell
                cells[cell_index]:render_children()
            end
        end
    end
end
cells[selected].object:add_state(lvgl.STATE.EDITED)

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

    -- change notes
    local is_capital_number = false
    for _, capital_key in ipairs(capital_keys) do
        if key == capital_key then is_capital_number = true break end
    end
    if is_capital_number then
        selected_cell.value = "."
        local number = capital_keys_map[key]
        if selected_cell.notes[number] then selected_cell.notes[number] = false
        else selected_cell.notes[number] = true end
        selected_cell:render_children()
        selected_cell:setSelected(true)
    end

    -- change value
    local is_small_number = false
    for _, small_key in ipairs(small_keys) do
        if key == small_key then is_small_number = true break end
    end
    if is_small_number then
        selected_cell.notes = {}
        local number = small_keys_map[key]
        if selected_cell.value == number then
            selected_cell.value = "."
            remaining = remaining + 1
        else 
            selected_cell.value = number
            remaining = remaining - 1

            -- clear notes by sudoku
            local selected_coords = cells:coordinates(selected)
            for _, cell in ipairs(cells:row(selected_coords.row)) do
                if cell.notes[number] then 
                    cell.notes[number] = false 
                    cell:render_children()  
                end
            end
            for _, cell in ipairs(cells:col(selected_coords.col)) do
                if cell.notes[number] then 
                    cell.notes[number] = false 
                    cell:render_children()
                end
            end
            for _, cell in ipairs(cells:box(selected_coords.row, selected_coords.col)) do
                if cell.notes[number] then 
                    cell.notes[number] = false 
                    cell:render_children()
                end
            end
        end
        selected_cell:render_children()
        selected_cell:setSelected(true)

        -- check if the sudoku is solved
        if remaining == 0 then
            print("Checking if the sudoku is solved")
            for i = 1, 9 do
                local values = {}
                for _, cell in ipairs(cells:row(i)) do
                    if values[cell.value] then return end
                    values[cell.value] = true
                end

                values = {}
                for _, cell in ipairs(cells:col(i)) do
                    if values[cell.value] then return end
                    values[cell.value] = true
                end

                values = {}
                local coords = cells:coordinates(i * 9)
                for _, cell in ipairs(cells:box(coords.row, coords.col)) do
                    if values[cell.value] then return end
                    values[cell.value] = true
                end

                print("Sudoku solved")
            end
        end
    end
end)
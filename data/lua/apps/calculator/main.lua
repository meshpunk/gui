-- Create a simple hello world label
local root = lvgl.Object()
root:set { w = lvgl.HOR_RES(), h = lvgl.VER_RES() }
root:clear_flag(lvgl.FLAG.SCROLLABLE)

-- flex layout and align
root:set {
    w = lvgl.HOR_RES(),
    h = lvgl.VER_RES(),
    align = lvgl.ALIGN.TOP_LEFT,
    pad_all = 0,
    border_width = 0,
}

-- Calculator State
local calc_state = {
  current = "0",
  operator = nil,
  operand = nil,
  reset_next = false
}

-- Vertical layout
local view = root:Object {
    flex = {
        flex_direction = "column",
        flex_wrap = "nowrap",
        align = lvgl.ALIGN.TOP_LEFT,
    },
    border_width = 0,
    h = lvgl.PCT(100),
    w = lvgl.PCT(100),
    pad_all = 10
}

-- Display
local display = view:Label {
    text = calc_state.current,
    w = lvgl.PCT(100),
    h = 40,
    pad_left = 10,
    pad_right = 10,
    pad_top = 6,
    pad_bottom = 6,
}

-- Container for buttons
local button_grid = view:Object {
    flex = {
        flex_direction = "row",
        flex_wrap = "wrap",
    },
    border_width = 0,
    w = lvgl.PCT(100),
    h = lvgl.VER_RES() - 50,
    pad_all = 10
}

-- Button click logic
local function update_display()
  print(calc_state.current)
  display.text = calc_state.current
end

local function perform_op()
  local a = tonumber(calc_state.operand)
  local b = tonumber(calc_state.current)
  local op = calc_state.operator

  if not a or not b or not op then return end

  if op == "+" then a = a + b
  elseif op == "-" then a = a - b
  elseif op == "*" then a = a * b
  elseif op == "/" then
    if b == 0 then
      calc_state.current = "Err"
      update_display()
      return
    else
      a = a / b
    end
  end

  calc_state.current = tostring(a)
  calc_state.operator = nil
  calc_state.operand = nil
  calc_state.reset_next = true
  update_display()
end

local function on_button_click(val)
  print("Button clicked!")
  print(val)

  if val:match("%d") then
    if calc_state.reset_next or calc_state.current == "0" then
      calc_state.current = val
      calc_state.reset_next = false
    else
      calc_state.current = calc_state.current .. val
    end
  elseif val == "=" then
    perform_op()
  elseif val == "+" or val == "-" or val == "*" or val == "/" then
    if calc_state.operator then
      perform_op()
    end
    calc_state.operand = calc_state.current
    calc_state.operator = val
    calc_state.reset_next = true
  end
  update_display()
end

-- Add buttons
local buttons = {
  "7", "8", "9", "/",
  "4", "5", "6", "*",
  "1", "2", "3", "-",
  "0", "=", "+"
}

local function createBtn(parent, value)
    -- make button 20% wide
    local btn = parent:Button {
        w = lvgl.PCT(20),
        h = 40
    }

    -- btn:onClicked(function()
    --     print("Button clicked!")
    --     -- root:set { bg_color = "#00FF00" }
    -- end)

    btn:Label {
        text = value,
        align = lvgl.ALIGN.CENTER,
    }

    btn:onClicked(function()
      on_button_click(value)
    end)

    return btn
end


for _, label in ipairs(buttons) do
  createBtn(button_grid, label)
end
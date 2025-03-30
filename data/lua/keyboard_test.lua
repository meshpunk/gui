-- Simple keyboard test
local root = lvgl.Object()
root:set { 
    w = lvgl.HOR_RES(), 
    h = lvgl.VER_RES(),
    bg_color = "#ffffff"
}

-- Create a button that we can focus
local btn = root:Button {
    text = "Press me",
    align = lvgl.ALIGN.CENTER,
    w = 100,
    h = 40
}

print("Setting up keyboard test")

-- Make button focusable and clickable
btn:add_flag(lvgl.FLAG.CLICKABLE)
btn:add_flag(lvgl.FLAG.FOCUSABLE)

-- Create a group for keyboard input
local group = lvgl.group.create()
group:add_obj(btn)  -- Add button to group instead of root
group:set_default()

print("Created input group and added button")

-- Get the keyboard input device and connect it to our group
local keyboard = lvgl.indev.get_next()
if keyboard then
    print("Found keyboard input device")
    keyboard:set_group(group)
end

-- Add keyboard event handler to button
btn:onevent(lvgl.EVENT.KEY, function(obj, code)
    local indev = lvgl.indev.get_act()
    if indev then
        local key = indev:get_key()
        -- Map special keys to readable names
        local key_name = ""
        if key == lvgl.KEY.ENTER then
            key_name = "ENTER"
        elseif key == lvgl.KEY.BACKSPACE then
            key_name = "BACKSPACE"
        elseif key == lvgl.KEY.ESC then
            key_name = "ESC"
        elseif key == lvgl.KEY.LEFT then
            key_name = "LEFT"
        elseif key == lvgl.KEY.RIGHT then
            key_name = "RIGHT"
        elseif key == lvgl.KEY.UP then
            key_name = "UP"
        elseif key == lvgl.KEY.DOWN then
            key_name = "DOWN"
        elseif key == lvgl.KEY.NEXT then
            key_name = "TAB"
        elseif key == lvgl.KEY.PREV then
            key_name = "SHIFT+TAB"
        else
            -- For regular characters, convert to string
            key_name = string.char(key)
        end
        print("Key pressed: " .. key_name)
    end
end)

-- Add focus event handlers to button
btn:onevent(lvgl.EVENT.FOCUSED, function(obj, code)
    print("Button received focus")
end)

btn:onevent(lvgl.EVENT.DEFOCUSED, function(obj, code)
    print("Button lost focus")
end)

-- Add click event to button
btn:onevent(lvgl.EVENT.CLICKED, function(obj, code)
    print("Button clicked!")
end)

print("Added all event handlers")

return root 
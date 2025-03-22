# LUAVGL Documentation

LUAVGL (Lua + LVGL) is a Lua binding for the Light and Versatile Graphics Library (LVGL), enabling the creation of rich GUIs using Lua scripting. This documentation covers how to set up and use luavgl in your projects.

## Table of Contents

1. [Introduction](#introduction)
2. [Setup](#setup)
    - [Embedded Devices](#embedded-devices)
    - [PC Simulator](#pc-simulator)
3. [Core Concepts](#core-concepts)
    - [Object Hierarchy](#object-hierarchy)
    - [Property Setting](#property-setting)
    - [Events](#events)
4. [LVGL Objects](#lvgl-objects)
    - [Creating Objects](#creating-objects)
    - [Basic Object Properties](#basic-object-properties)
    - [Object Methods](#object-methods)
5. [Widgets](#widgets)
    - [Button](#button)
    - [Label](#label)
    - [Image](#image)
    - [Checkbox](#checkbox)
    - [Dropdown](#dropdown)
    - [Calendar](#calendar)
    - [Keyboard](#keyboard)
    - [List](#list)
    - [Roller](#roller)
    - [Textarea](#textarea)
    - [LED](#led)
6. [Styling](#styling)
    - [Creating Styles](#creating-styles)
    - [Applying Styles](#applying-styles)
7. [Layouts](#layouts)
    - [Flex Layout](#flex-layout)
    - [Grid Layout](#grid-layout)
8. [Animations](#animations)
9. [Fonts](#fonts)
10. [Events & Callbacks](#events-callbacks)
11. [Input Devices](#input-devices)
12. [Custom Widgets](#custom-widgets)
13. [Example Projects](#example-projects)

## Introduction

LUAVGL is a wrapper around LVGL core functions and widgets with class inheritance in mind. It's designed to make GUI development more accessible through Lua scripting, primarily targeting embedded devices but also usable on PC simulators.

Key features:
- Object-oriented API with widget inheritance
- Comprehensive support for LVGL core features
- Property-based attribute setting
- Event handling with Lua callbacks
- Animation support
- Flex and grid layout systems

## Setup

### Embedded Devices

For embedded devices, LVGL must be set up before using LUAVGL. Once LVGL and a Lua interpreter are running, follow these steps:

1. Add `luavgl.c` to your project sources for compilation
2. Register the Lua module:

```c
/* add `lvgl` module to global package table */
luaL_requiref(L, "lvgl", luaopen_luavgl, 1);
lua_pop(L, 1);
```

3. Set the root object for the LVGL UI:

```c
lv_obj_t *root = lv_obj_create(lv_scr_act());
luavgl_set_root(L, root);
```

4. (Optional) Set custom font handling:

```c
luavgl_set_font_extension(L, my_font_maker, my_font_destroyer);
```

### PC Simulator

For PC development, you can use the provided simulator:

1. Clone the LUAVGL repository with submodules:
```bash
git clone --recursive https://github.com/XuNeo/luavgl.git
```

2. Build with CMake:
```bash
cmake -Bbuild -DBUILD_SIMULATOR=ON
cd build
make
make run # run simulator
```

Or with xmake:
```bash
xmake b simulator
xmake r
```

## Core Concepts

### Object Hierarchy

LUAVGL uses LVGL's object hierarchy, where widgets are created on parent objects. The root object is typically the screen or a container.

```lua
-- Create a root object
local root = lvgl.Object()
root:set { w = lvgl.HOR_RES(), h = lvgl.VER_RES() }

-- Create a child object
local child = root:Object()
```

### Property Setting

Properties are set using the `:set` method with a table of key-value pairs:

```lua
object:set {
    x = 10,
    y = 20,
    w = 100,
    h = 50,
    bg_color = "#FF0000",
    border_width = 2
}
```

### Events

Events are handled with Lua callbacks:

```lua
button:add_event(function(e)
    print("Button clicked!")
end, lvgl.EVENT.CLICKED)
```

## LVGL Objects

### Creating Objects

Objects are created by calling the constructor method on a parent object:

```lua
-- Create a base object
local root = lvgl.Object()

-- Create a child object
local child = root:Object()
```

### Basic Object Properties

Common object properties:

```lua
obj:set {
    x = 10,                    -- X position
    y = 10,                    -- Y position
    w = 100,                   -- Width
    h = 100,                   -- Height
    align = lvgl.ALIGN.CENTER, -- Alignment
    pad_all = 10,              -- Padding
    bg_color = "#FF0000",      -- Background color
    border_width = 2,          -- Border width
    border_color = "#000000",  -- Border color
    radius = 10                -- Corner radius
}
```

### Object Methods

Common methods for all objects:

```lua
-- Add event callback
obj:add_event(callback, event_code)

-- Add/remove flags
obj:add_flag(lvgl.FLAG.CLICKABLE)
obj:clear_flag(lvgl.FLAG.SCROLLABLE)

-- Add/remove states
obj:add_state(lvgl.STATE.CHECKED)
obj:clear_state(lvgl.STATE.DISABLED)

-- Check if object has specific state
local has_state = obj:has_state(lvgl.STATE.PRESSED)

-- Property setters and getters
obj:set_x(10)
obj:set_y(20)
local x = obj:get_x()
local y = obj:get_y()

-- Delete object and its children
obj:delete()  -- deletes this object and all its children

-- Remove all children but keep the object
obj:clean()   -- removes all child objects but keeps the parent
```

## Widgets

### Button

```lua
local button = parent:Button()
button:set {
    text = "Click me",
    w = 100,
    h = 50,
    align = lvgl.ALIGN.CENTER
}

button:add_event(function(e)
    print("Button clicked!")
end, lvgl.EVENT.CLICKED)
```

### Label

```lua
local label = parent:Label {
    text = "Hello LVGL",
    align = lvgl.ALIGN.CENTER,
    text_font = lvgl.Font("montserrat", 24, "normal"),
    -- Or use built-in font:
    -- text_font = lvgl.BUILTIN_FONT.MONTSERRAT_24
}

-- Update label text
label:set_text("Updated text")
```

### Image

```lua
local img = parent:Image {
    src = "path/to/image.png",
    align = lvgl.ALIGN.CENTER
}

-- Create animation for image
img:Anim {
    run = true,
    start_value = 0,
    end_value = 3600,
    duration = 2000,
    repeat_count = 2,
    path = "bounce",
    exec_cb = function(obj, value)
        obj:set { angle = value }
    end
}
```

### Checkbox

```lua
local cb = parent:Checkbox {
    text = "Option",
    align = lvgl.ALIGN.CENTER
}

cb:add_event(function(e)
    print("Checkbox state:", cb:is_checked())
end, lvgl.EVENT.VALUE_CHANGED)
```

### Dropdown

```lua
local dd = parent:Dropdown {
    options = "Option 1\nOption 2\nOption 3",
    align = lvgl.ALIGN.CENTER
}

dd:add_event(function(e)
    print("Selected:", dd:get_selected())
end, lvgl.EVENT.VALUE_CHANGED)
```

### Calendar

```lua
local cal = parent:Calendar {
    align = lvgl.ALIGN.CENTER,
    today_date = {year = 2023, month = 6, day = 15}
}

cal:add_event(function(e)
    local date = cal:get_pressed_date()
    print(string.format("Date: %d-%02d-%02d", 
        date.year, date.month, date.day))
end, lvgl.EVENT.VALUE_CHANGED)
```

### Keyboard

```lua
local kb = parent:Keyboard {
    mode = lvgl.KEYBOARD_MODE.TEXT_LOWER,
    align = lvgl.ALIGN.BOTTOM_MID
}

-- Connect to a textarea
kb:set_textarea(textarea)
```

### List

```lua
local list = parent:List {
    align = lvgl.ALIGN.CENTER,
    w = 200,
    h = 200
}

-- Add buttons to the list
list:add_btn("icon.png", "Item 1")
list:add_btn(nil, "Item 2")
list:add_btn(nil, "Item 3")
```

### Roller

```lua
local roller = parent:Roller {
    options = "Option 1\nOption 2\nOption 3\nOption 4\nOption 5",
    visible_row_count = 3,
    align = lvgl.ALIGN.CENTER
}

roller:add_event(function(e)
    print("Selected:", roller:get_selected())
end, lvgl.EVENT.VALUE_CHANGED)
```

### Textarea

```lua
local ta = parent:Textarea {
    placeholder_text = "Type here...",
    w = 200,
    h = 100,
    align = lvgl.ALIGN.CENTER
}

ta:add_event(function(e)
    print("Text:", ta:get_text())
end, lvgl.EVENT.VALUE_CHANGED)
```

### LED
```lua
local led = parent:Led {
    align = lvgl.ALIGN.CENTER
}

-- Set brightness (0-255)
led:set_brightness(150)
led:toggle()
```

## Styling

### Creating Styles

```lua
local style = lvgl.Style()
style:set {
    bg_color = "#FF0000",
    border_width = 2,
    border_color = "#000000",
    pad_all = 10,
    text_color = "#FFFFFF",
    radius = 10
}
```

### Applying Styles

```lua
-- Apply to an object
obj:add_style(style, lvgl.STATE.DEFAULT)

-- Apply to a specific part
button:add_style(style, lvgl.STATE.DEFAULT, lvgl.PART.MAIN)
```

## Layouts

### Flex Layout

```lua
container:set {
    flex = {
        flex_direction = "row", -- "row", "column", "row_wrap", "column_wrap"
        flex_wrap = "wrap",     -- "nowrap", "wrap", "wrap_reverse"
        justify_content = "center", -- "start", "end", "center", "space_between", "space_around", "space_evenly"
        align_items = "center", -- "start", "end", "center", "stretch" 
        align_content = "center" -- "start", "end", "center", "stretch", "space_between", "space_around"
    },
    w = 300,
    h = 200
}

-- Set flex grow value for a child
child:set_flex_grow(1)
```

### Grid Layout

```lua
container:set {
    grid = {
        column_dsc = "100px 1fr 100px",
        row_dsc = "50px 1fr 50px",
    }
}

-- Place child in grid
child:set_grid_cell(1, 1, 1, 1) -- col, row, col_span, row_span
```

## Animations

```lua
-- Create animation
obj:Anim {
    run = true,             -- Start immediately
    start_value = 0,        -- Start value
    end_value = 100,        -- End value
    duration = 1000,        -- Duration in ms
    delay = 0,              -- Delay before start
    repeat_count = 0,       -- 0 for infinite
    repeat_delay = 0,       -- Delay between repeats
    early_apply = false,    -- Apply first value immediately
    path = "linear",        -- "linear", "ease_in", "ease_out", "ease_in_out", "bounce"
    exec_cb = function(obj, value)
        obj:set_x(value)    -- Animation callback
    end,
    ready_cb = function()   -- Called when animation completes
        print("Animation done")
    end
}
```

## Fonts

```lua
-- Using built-in fonts
local builtin_font = lvgl.BUILTIN_FONT.MONTSERRAT_24

-- Using custom fonts (requires font extension to be set up)
local custom_font = lvgl.Font("montserrat", 24, "normal")
-- Parameters: name, size, weight ("normal", "medium", "bold")

-- Apply font to an object
label:set {
    text_font = custom_font
}
```

## Events & Callbacks

```lua
-- Add event callback
obj:add_event(function(e)
    -- Event data is available in the event object
    local code = e:get_code()
    local target = e:get_target()
    
    if code == lvgl.EVENT.CLICKED then
        print("Object clicked")
    end
end, lvgl.EVENT.CLICKED)

-- Add multiple event codes
obj:add_event(function(e)
    local code = e:get_code()
    if code == lvgl.EVENT.PRESSED then
        print("Pressed")
    elseif code == lvgl.EVENT.RELEASED then
        print("Released")
    end
end, lvgl.EVENT.PRESSED, lvgl.EVENT.RELEASED)

-- Remove event
obj:remove_event(event_ref)
```

## Input Devices

```lua
-- Get indev (requires LVGL to be properly initialized)
local indev = lvgl.get_indev()

-- Set cursor
indev:set_cursor(cursor_obj)

-- Set group
indev:set_group(group)
```

## Custom Widgets

You can extend LUAVGL with custom widgets by implementing them in C and registering them with the Lua environment. See the documentation in the source code for details.

```c
// In C code
static const luaL_Reg my_widget_methods[] = {
    {"MyWidget", my_widget_create},
    {NULL, NULL},
};

void register_my_widget(lua_State *L) {
    // Register methods
    lua_pushstring(L, "MyWidget");
    lua_pushcfunction(L, my_widget_create);
    lua_rawset(L, -3);
}
```

## Example Projects

### Simple Button Example

```lua
local root = lvgl.Object()
root:set { w = lvgl.HOR_RES(), h = lvgl.VER_RES() }

local button = root:Button {
    text = "Click Me!",
    w = 150,
    h = 50,
    align = lvgl.ALIGN.CENTER
}

local counter = 0
button:add_event(function(e)
    counter = counter + 1
    button:set {
        text = "Clicks: " .. counter
    }
end, lvgl.EVENT.CLICKED)
```

### Flex Layout Example

```lua
local root = lvgl.Object()
root:set { w = lvgl.HOR_RES(), h = lvgl.VER_RES() }

-- Create a flex container
local container = root:Object {
    w = 300,
    h = 200,
    align = lvgl.ALIGN.CENTER,
    flex = {
        flex_direction = "row",
        flex_wrap = "wrap",
        justify_content = "space_evenly",
        align_items = "center"
    }
}

-- Add items to the flex container
for i = 1, 5 do
    local item = container:Object {
        w = 80,
        h = 50,
        bg_color = i % 2 == 0 and "#3366FF" or "#FF6633"
    }
    
    item:Label {
        text = "Item " .. i,
        align = lvgl.ALIGN.CENTER
    }
end
```

### Animated Image Example

```lua
local root = lvgl.Object()
root:set { w = lvgl.HOR_RES(), h = lvgl.VER_RES() }

local img = root:Image {
    src = "logo.png",
    align = lvgl.ALIGN.CENTER
}

-- Create a rotation animation
img:Anim {
    run = true,
    start_value = 0,
    end_value = 3600,
    duration = 10000,
    repeat_count = 0, -- infinite
    path = "linear",
    exec_cb = function(obj, value)
        obj:set { angle = value }
    end
}
```

For more examples, check the LUAVGL repository and the provided simulator code. 

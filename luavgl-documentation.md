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
    - [Units and Measurements](#units-and-measurements)
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
    - [Style Properties](#style-properties)
    - [Applying Styles](#applying-styles)
    - [Style Inheritance](#style-inheritance)
    - [Style Combinations](#style-combinations)
    - [Removing Styles](#removing-styles)
    - [Example: Complex Styling](#example-complex-styling)
7. [Layouts](#layouts)
    - [Flex Layout](#flex-layout)
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

## Units and Measurements

LUAVGL uses a consistent system for handling units and measurements across all widgets and layouts. Here's a detailed breakdown:

### Basic Units
- All position and size values are handled as pixels through `lv_coord_t` (an integer type)
- Width, height, x, and y coordinates are all specified in pixels
- Example:
```lua
obj:set({
    x = 100,        -- 100 pixels from left
    y = 50,         -- 50 pixels from top
    width = 200,    -- 200 pixels wide
    height = 150    -- 150 pixels tall
})
```

### Coordinate Limits
LUAVGL provides special constants for defining the boundaries of the coordinate system:
- `lvgl.COORD_MAX`: Maximum possible coordinate value for dimensions and positions
- `lvgl.COORD_MIN`: Minimum possible coordinate value for dimensions and positions

These constants are useful for:
- Setting objects to their maximum/minimum possible size
- Defining boundaries for scrolling or positioning
- Working with layout calculations

Example:
```lua
-- Make an object span the full available width
obj:set {
    width = lvgl.COORD_MAX,
    height = 100  -- Fixed height of 100 pixels
}

-- Create a minimal-sized object
obj:set {
    width = lvgl.COORD_MIN,
    height = lvgl.COORD_MIN
}
```

### Percentage Values
- Use `PCT()` function to specify dimensions as percentages of the parent container
- Internally implemented as `luavgl_LV_PCT()` which uses the LVGL macro `LV_PCT()`
- Example:
```lua
obj:set({
    width = lvgl.PCT(50),   -- 50% of parent width
    height = lvgl.PCT(100)  -- 100% of parent height
})
```

### Special Size Values
LUAVGL provides special constants for flexible sizing:
- `SIZE_CONTENT`: Size the object to fit its content
- `COORD_MAX`: Maximum possible coordinate value
- `COORD_MIN`: Minimum possible coordinate value

Example:
```lua
obj:set({
    width = lvgl.SIZE_CONTENT,  -- Fit to content width
    height = lvgl.SIZE_CONTENT  -- Fit to content height
})
```

### Position and Alignment
- Basic positioning uses pixel coordinates from top-left
- Alignment can be specified using predefined constants or tables with offsets
- Simple alignment:
```lua
obj:set({
    align = lvgl.ALIGN.CENTER  -- Center in parent
})
```
- Complex alignment with offset:
```lua
obj:set({
    align = {
        type = lvgl.ALIGN.CENTER,
        x_ofs = 10,  -- 10 pixels right offset
        y_ofs = -5   -- 5 pixels up offset
    }
})
```

Available alignment constants:
- `lvgl.ALIGN.DEFAULT`
- `lvgl.ALIGN.TOP_LEFT`
- `lvgl.ALIGN.TOP_MID`
- `lvgl.ALIGN.TOP_RIGHT`
- `lvgl.ALIGN.BOTTOM_LEFT`
- `lvgl.ALIGN.BOTTOM_MID`
- `lvgl.ALIGN.BOTTOM_RIGHT`
- `lvgl.ALIGN.LEFT_MID`
- `lvgl.ALIGN.RIGHT_MID`
- `lvgl.ALIGN.CENTER`
- `lvgl.ALIGN.OUT_TOP_LEFT`
- `lvgl.ALIGN.OUT_TOP_MID`
- `lvgl.ALIGN.OUT_TOP_RIGHT`
- `lvgl.ALIGN.OUT_BOTTOM_LEFT`
- `lvgl.ALIGN.OUT_BOTTOM_MID`
- `lvgl.ALIGN.OUT_BOTTOM_RIGHT`
- `lvgl.ALIGN.OUT_LEFT_TOP`
- `lvgl.ALIGN.OUT_LEFT_MID`
- `lvgl.ALIGN.OUT_LEFT_BOTTOM`
- `lvgl.ALIGN.OUT_RIGHT_TOP`
- `lvgl.ALIGN.OUT_RIGHT_MID`
- `lvgl.ALIGN.OUT_RIGHT_BOTTOM`

### Screen Resolution
Functions to get screen dimensions for responsive design:
- `lvgl.HOR_RES()`: Get screen width in pixels
- `lvgl.VER_RES()`: Get screen height in pixels

Example:
```lua
local screen_width = lvgl.HOR_RES()
local screen_height = lvgl.VER_RES()
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
-- Property setting
obj:set { property = value }  -- Set multiple properties
obj:get("property")          -- Get a property value

-- Event handling
obj:add_event(callback, event_code)  -- Add event callback
obj:onevent(callback, event_code)    -- Alternative event handler
obj:onPressed(callback)              -- Handle pressed event
obj:onClicked(callback)              -- Handle clicked event
obj:onShortClicked(callback)         -- Handle short click event

-- Object hierarchy
obj:set_parent(parent)       -- Set object's parent
obj:get_parent()            -- Get object's parent
obj:get_screen()            -- Get object's screen
obj:get_child(index)        -- Get child by index
obj:get_child_cnt()         -- Get number of children
obj:get_child_by_id(id)     -- Get child by ID
obj:delete()                -- Delete object and its children
obj:clean()                 -- Remove all children but keep object

-- State and flags
obj:add_flag(lvgl.FLAG.CLICKABLE)     -- Add flag
obj:clear_flag(lvgl.FLAG.SCROLLABLE)  -- Clear flag
obj:add_state(lvgl.STATE.CHECKED)     -- Add state
obj:clear_state(lvgl.STATE.DISABLED)  -- Clear state
obj:get_state()                       -- Get current state
obj:is_visible()                      -- Check if object is visible
obj:is_editable()                     -- Check if object is editable
obj:is_group_def()                    -- Check if object is group defocusable
obj:is_layout_positioned()            -- Check if object is positioned by layout

-- Styling
obj:add_style(style, state)           -- Add style
obj:remove_style(style)               -- Remove style
obj:remove_style_all()                -- Remove all styles

-- Layout
obj:set_flex_flow(flow)               -- Set flex flow
obj:set_flex_align(align)             -- Set flex alignment
obj:set_flex_grow(grow)               -- Set flex grow
obj:mark_layout_as_dirty()            -- Mark layout for recalculation
obj:center()                          -- Center object in parent

-- Position and size
obj:align_to({               -- Align relative to another object
    base = other_obj,
    type = lvgl.ALIGN.CENTER,
    x_ofs = 0,
    y_ofs = 0
})
obj:get_coords()            -- Get coordinates {x1,y1,x2,y2}
obj:get_pos()              -- Get position {x1,y1,x2,y2}

-- Scrolling
obj:scroll_to({ x = 0, y = 0, anim = true })  -- Scroll to position
obj:scroll_by(x, y, anim_en)                  -- Scroll by offset
obj:scroll_by_bounded(dx, dy, anim_en)        -- Scroll with bounds
obj:scroll_to_view(anim_en)                   -- Scroll object into view
obj:scroll_to_view_recursive(anim_en)         -- Scroll into view recursively
obj:is_scrolling()                            -- Check if object is scrolling
obj:scrollbar_invalidate()                    -- Invalidate scrollbars
obj:readjust_scroll(anim_en)                 -- Readjust scroll position

-- Animation
obj:Anim({                  -- Create animation
    start_value = 0,
    end_value = 100,
    duration = 1000,
    exec_cb = function(obj, value) end
})
obj:remove_all_anim()       -- Remove all animations

-- Input handling
obj:indev_search()          -- Search for input device

-- Drawing
obj:invalidate()            -- Force redraw
```

### States and Flags

LVGL objects can have different states and flags that control their behavior and appearance.

#### States

States represent the current condition of an object. Multiple states can be active at once.

```lua
-- Available states (lvgl.STATE.*)
local states = {
    DEFAULT = 0,            -- Default state
    CHECKED = 1,           -- Object is checked/selected
    FOCUSED = 2,           -- Object is focused (e.g. via keyboard)
    FOCUS_KEY = 4,         -- Object is focused via keyboard
    EDITED = 8,            -- Object is being edited
    HOVERED = 16,          -- Mouse cursor is over the object
    PRESSED = 32,          -- Object is being pressed
    SCROLLED = 64,         -- Object is being scrolled
    DISABLED = 128,        -- Object is disabled/inactive
    USER_1 = 256,          -- Custom state 1
    USER_2 = 512,          -- Custom state 2
    USER_3 = 1024,         -- Custom state 3
    USER_4 = 2048          -- Custom state 4
}

-- Managing states
obj:add_state(lvgl.STATE.CHECKED)      -- Add a state
obj:clear_state(lvgl.STATE.DISABLED)   -- Remove a state
local state = obj:get_state()          -- Get current state

-- Example: Style based on state
local style = lvgl.Style {bg_color = "#FF0000"}
obj:add_style(style, lvgl.STATE.CHECKED)
```

##### Understanding USER States

LVGL provides four custom states (USER_1 through USER_4) that integrate with LVGL's state-based styling system. These are different from adding custom fields to objects and offer several advantages:

Key Benefits:
1. **Style Integration**: USER states work directly with LVGL's styling system
2. **Performance**: State changes trigger LVGL's optimized style system
3. **Consistency**: Works with LVGL's built-in state handling
4. **Transitions**: Can use LVGL's style transitions between states
5. **Combinations**: Can be combined with other states using bitwise operations
6. **Event Integration**: State changes trigger LVGL events automatically

Example using USER states for styling:
```lua
-- Create styles for different states
local style_active = lvgl.Style()
style_active:set {
    bg_color = "#FF0000",
    border_width = 2
}

local style_inactive = lvgl.Style()
style_inactive:set {
    bg_color = "#888888",
    border_width = 0
}

-- Apply styles based on USER_1 state
obj:add_style(style_active, lvgl.STATE.USER_1)
obj:add_style(style_inactive, ~lvgl.STATE.USER_1)

-- Switch between states
obj:add_state(lvgl.STATE.USER_1)    -- Activates the active style
obj:clear_state(lvgl.STATE.USER_1)  -- Activates the inactive style
```

Combining with built-in states:
```lua
-- Style applies when both conditions are true
obj:add_style(style1, lvgl.STATE.USER_1 | lvgl.STATE.PRESSED)
obj:add_style(style2, lvgl.STATE.USER_2 | lvgl.STATE.DISABLED)
```

Common Use Cases:

1. Custom Tab States:
```lua
local TAB_STATES = {
    NORMAL = lvgl.STATE.DEFAULT,
    SELECTED = lvgl.STATE.USER_1,
    HIGHLIGHTED = lvgl.STATE.USER_2
}

tab:add_style(normal_style, TAB_STATES.NORMAL)
tab:add_style(selected_style, TAB_STATES.SELECTED)
tab:add_style(highlight_style, TAB_STATES.HIGHLIGHTED)
```

2. Game States:
```lua
local GAME_STATES = {
    PLAYING = lvgl.STATE.USER_1,
    PAUSED = lvgl.STATE.USER_2,
    GAME_OVER = lvgl.STATE.USER_3
}

game_ui:add_style(playing_style, GAME_STATES.PLAYING)
game_ui:add_style(paused_style, GAME_STATES.PAUSED)
game_ui:add_style(game_over_style, GAME_STATES.GAME_OVER)
```

When to Use USER States vs Custom Fields:

Use USER states when:
- You need style changes based on the state
- You want to combine with other LVGL states
- You need transition animations between states
- You want to trigger state-change events

Use custom fields when:
- You need to store data not related to appearance
- You need more than 4 custom states
- You need to store non-boolean values
- The state doesn't affect the object's appearance

Example combining both approaches:
```lua
-- Game object that needs both visual states and data
local game_object = parent:Object()

-- Visual states using USER states
local VISUAL_STATES = {
    NORMAL = lvgl.STATE.DEFAULT,
    HIGHLIGHTED = lvgl.STATE.USER_1,
    SELECTED = lvgl.STATE.USER_2
}

-- Data using custom fields
game_object.score = 0
game_object.player_name = "Player 1"
game_object.position = { x = 0, y = 0 }

-- Using both together
game_object:add_event(function(e)
    if e.target:get_state() & VISUAL_STATES.SELECTED then
        -- Update score when selected
        game_object.score = game_object.score + 1
        -- Update appearance based on score
        if game_object.score > 10 then
            game_object:add_state(VISUAL_STATES.HIGHLIGHTED)
        end
    end
end, lvgl.EVENT.VALUE_CHANGED)
```

#### Flags

Flags control various object behaviors. Unlike states, flags are either on or off.

```lua
-- Available flags (lvgl.FLAG.*)
local flags = {
    HIDDEN = 1,            -- Object is hidden
    CLICKABLE = 2,         -- Object can receive click/touch events
    CLICK_FOCUSABLE = 4,   -- Object can be focused by clicking
    CHECKABLE = 8,         -- Object can be checked/unchecked
    SCROLLABLE = 16,       -- Object can be scrolled
    SCROLL_ELASTIC = 32,   -- Scroll is elastic
    SCROLL_MOMENTUM = 64,  -- Scroll has momentum
    SCROLL_ONE = 128,      -- Only one child can be scrolled at a time
    SCROLL_CHAIN = 256,    -- Scroll can be propagated to parent
    SCROLL_ON_FOCUS = 512, -- Object scrolls into view when focused
    SNAPPABLE = 1024,      -- Object snaps to grid
    PRESS_LOCK = 2048,     -- Press events are locked to this object
    EVENT_BUBBLE = 4096,   -- Events bubble to parent
    GESTURE_BUBBLE = 8192, -- Gestures bubble to parent
    ADV_HITTEST = 16384,   -- Advanced hit-testing
    IGNORE_LAYOUT = 32768, -- Object ignores layout rules
    FLOATING = 65536,      -- Object is floating (ignores layout)
    OVERFLOW_VISIBLE = 131072  -- Children can be visible outside parent
}

-- Managing flags
obj:add_flag(lvgl.FLAG.CLICKABLE)      -- Add a flag
obj:clear_flag(lvgl.FLAG.SCROLLABLE)   -- Remove a flag

-- Example: Common flag combinations
obj:add_flag(lvgl.FLAG.CLICKABLE + lvgl.FLAG.CHECKABLE)  -- Make object clickable and checkable

-- Example: Make object interactive
obj:add_flag(
    lvgl.FLAG.CLICKABLE +
    lvgl.FLAG.CLICK_FOCUSABLE +
    lvgl.FLAG.SCROLLABLE +
    lvgl.FLAG.SCROLL_MOMENTUM
)
```

Common use cases:
1. Making an object interactive:
```lua
obj:add_flag(lvgl.FLAG.CLICKABLE + lvgl.FLAG.CLICK_FOCUSABLE)
```

2. Setting up a scrollable container:
```lua
container:add_flag(
    lvgl.FLAG.SCROLLABLE +
    lvgl.FLAG.SCROLL_MOMENTUM +
    lvgl.FLAG.SCROLL_ELASTIC
)
```

3. Creating a checkable button:
```lua
button:add_flag(lvgl.FLAG.CLICKABLE + lvgl.FLAG.CHECKABLE)
button:add_event(function(e)
    if button:get_state() & lvgl.STATE.CHECKED then
        print("Button is checked")
    end
end, lvgl.EVENT.VALUE_CHANGED)
```

4. Making an object float above layout:
```lua
overlay:add_flag(lvgl.FLAG.FLOATING + lvgl.FLAG.IGNORE_LAYOUT)
```

5. Setting up event propagation:
```lua
parent:add_flag(lvgl.FLAG.EVENT_BUBBLE + lvgl.FLAG.GESTURE_BUBBLE)
```

## Widgets

### Button

The Button widget is a simple clickable object that inherits all functionality from the base object. It's commonly used for user interactions.

```lua
-- Create a basic button
local button = parent:Button {
    w = 100,                    -- Width
    h = 40,                     -- Height
    align = lvgl.ALIGN.CENTER   -- Alignment
}

-- Add a label to the button
button:Label {
    text = "Click me",
    align = lvgl.ALIGN.CENTER
}

-- Make it interactive
button:add_flag(lvgl.FLAG.CLICKABLE)

-- Handle clicks
button:add_event(function(e)
    print("Button clicked!")
end, lvgl.EVENT.CLICKED)

-- Example: Create a styled button
local style = lvgl.Style()
style:set {
    bg_color = "#2196F3",      -- Blue background
    bg_opa = 255,              -- Full opacity
    border_width = 0,          -- No border
    radius = 8,                -- Rounded corners
    pad_all = 10,              -- Padding
    text_color = "#FFFFFF",    -- White text
    text_font = lvgl.BUILTIN_FONT.MONTSERRAT_14  -- Font
}

local styled_button = parent:Button {
    w = 120,
    h = 40,
    align = lvgl.ALIGN.CENTER
}
styled_button:add_style(style, lvgl.STATE.DEFAULT)

-- Add hover effect
local style_pressed = lvgl.Style()
style_pressed:set {
    bg_color = "#1976D2"  -- Darker blue when pressed
}
styled_button:add_style(style_pressed, lvgl.STATE.PRESSED)

-- Add label
styled_button:Label {
    text = "Styled Button",
    align = lvgl.ALIGN.CENTER
}
```

Key features:
- Simple clickable container
- Can contain other widgets (commonly used with Label)
- Supports all base object properties and methods
- Can be styled for different states (default, pressed, disabled, etc.)
- Automatically handles press/release animations when styled

Common use cases:
1. Basic click actions
2. Toggle buttons (using `CHECKABLE` flag)
3. Navigation controls
4. Form submissions
5. Menu items

### Label

The Label widget displays text with extensive formatting options. It's one of the most fundamental widgets in LUAVGL.

```lua
-- Create a basic label
local label = parent:Label {
    text = "Hello LVGL",
    align = lvgl.ALIGN.CENTER
}

-- Set text properties
label:set {
    text = "New text",                                    -- Change text content
    text_font = lvgl.BUILTIN_FONT.MONTSERRAT_14,         -- Set font
    text_color = "#FFFFFF",                              -- Text color
    text_align = lvgl.TEXT_ALIGN.CENTER,                 -- Text alignment within label
    text_line_space = 2,                                 -- Space between lines
    text_letter_space = 1                                -- Space between letters
}

-- Handle long text with different modes
label:set {
    w = 100,                                             -- Constrain width
    long_mode = lvgl.LABEL_LONG.WRAP,                    -- Wrap text to new lines
    -- Other long_mode options:
    -- lvgl.LABEL_LONG.DOT              -- Add ... at the end
    -- lvgl.LABEL_LONG.SCROLL           -- Scroll text horizontally
    -- lvgl.LABEL_LONG.SCROLL_CIRCULAR  -- Scroll continuously
    -- lvgl.LABEL_LONG.CLIP             -- Simply clip the text
}

-- Text manipulation methods
label:ins_text(0, "Prefix: ")                           -- Insert text at position
label:cut_text(0, 5)                                    -- Remove 5 characters from position 0

-- Get text content
local text = label:get_text()                           -- Get current text

-- Text selection (if enabled)
label:set {
    text_selection_start = 0,                           -- Start of selection
    text_selection_end = 5                              -- End of selection
}

-- Example: Create a styled label with animation
local style = lvgl.Style()
style:set {
    text_font = lvgl.BUILTIN_FONT.MONTSERRAT_20,
    text_color = "#2196F3",
    text_line_space = 5,
    text_letter_space = 2,
    pad_all = 10,
    bg_color = "#000000",
    bg_opa = 50,
    radius = 8
}

local animated_label = parent:Label {
    text = "Animated Text",
    align = lvgl.ALIGN.CENTER,
    w = 200
}
animated_label:add_style(style, lvgl.STATE.DEFAULT)

-- Add scrolling animation for long text
animated_label:set {
    long_mode = lvgl.LABEL_LONG.SCROLL_CIRCULAR,
    text = "This is a long text that will scroll continuously in a circular manner"
}
```

Key features:
- Text display with rich formatting options
- Multiple fonts support (built-in and custom)
- Text alignment (left, right, center)
- Line and letter spacing
- Long text handling modes (wrap, dot, scroll, clip)
- Text selection support
- Text manipulation (insert, cut)
- Automatic size calculation
- Scrolling animations for long text

Long text handling modes:
1. `WRAP` - Text wraps to next line when it reaches the width limit
2. `DOT` - Adds "..." at the end when text is too long
3. `SCROLL` - Scrolls text horizontally when it's too long
4. `SCROLL_CIRCULAR` - Continuously scrolls text in a circular manner
5. `CLIP` - Simply clips the text at the width limit

Common use cases:
1. Display static text (titles, descriptions)
2. Show dynamic content (values, status messages)
3. Create scrolling announcements
4. Build menu items
5. Show tooltips
6. Display error messages
7. Create text-based animations

### Image

The Image widget displays images in various formats supported by LVGL. It supports basic image manipulation like rotation, scaling, and positioning.

```lua
-- Create a basic image
local img = parent:Image {
    src = "path/to/image.png",    -- Image source path
    align = lvgl.ALIGN.CENTER     -- Alignment within parent
}

-- Set image properties
img:set {
    w = 200,                      -- Width (optional, defaults to content width)
    h = 150,                      -- Height (optional, defaults to content height)
    zoom = 256,                   -- Zoom level (256 = 100%)
    angle = 450,                  -- Rotation angle in tenths of a degree (450 = 45°)
    antialias = true,            -- Enable antialiasing for rotated images
    pivot = { x = 100, y = 75 }  -- Rotation center point
}

-- Set image offset (for partial display or animation)
img:set_offset {
    x = 10,                       -- X offset from original position
    y = 20                        -- Y offset from original position
}

-- Change image source
img:set_src("new_image.png")

-- Get image dimensions
local w, h = img:get_img_size()   -- Get current image size
local w, h = img:get_img_size("other_image.png")  -- Get size of another image

-- Example: Create an animated image with rotation
local style = lvgl.Style()
style:set {
    img_recolor = "#2196F3",      -- Recolor the image with blue
    img_recolor_opa = 128,        -- 50% recolor opacity
    img_opa = 255                 -- Full image opacity
}

local animated_img = parent:Image {
    src = "icon.png",
    align = lvgl.ALIGN.CENTER,
    w = 100,
    h = 100
}
animated_img:add_style(style, lvgl.STATE.DEFAULT)

-- Add rotation animation
animated_img:Anim {
    run = true,
    start_value = 0,
    end_value = 3600,            -- Full 360° rotation (in tenths of degrees)
    duration = 3000,             -- 3 seconds
    repeat_count = 0,            -- Infinite repetition
    path = "linear",             -- Linear animation path
    exec_cb = function(obj, value)
        obj:set { angle = value }
    end
}
```

Key features:
- Support for multiple image formats (PNG, JPG, BMP, etc., depending on LVGL configuration)
- Image rotation with custom pivot point
- Image offset control
- Image size querying
- Zoom control
- Antialiasing for rotated images
- Style properties for recoloring and opacity
- Animation support

Image source types:
1. File paths (e.g., "path/to/image.png")
2. Symbols from font (e.g., lvgl.SYMBOL.OK)
3. Variables containing image data
4. Online images (if LVGL is configured with network support)

Style properties specific to images:
- `img_opa`: Image opacity
- `img_recolor`: Color to blend with the image
- `img_recolor_opa`: Intensity of recoloring
- `transform_zoom`: Image zoom
- `transform_angle`: Rotation angle

### Checkbox

The Checkbox widget is a clickable object that can be checked/unchecked and displays text next to a checkbox indicator.

```lua
-- Create a basic checkbox
local cb = parent:Checkbox {
    text = "Option",              -- Text label for the checkbox
    align = lvgl.ALIGN.CENTER,    -- Alignment within parent
    checked = true                -- Initial checked state (optional)
}

-- Set checkbox properties
cb:set {
    text = "New text",            -- Change the checkbox text
}

-- Get checkbox text
local text = cb:get_text()

-- Handle state changes
cb:add_event(function(e)
    local state = e.target:get_state()
    local is_checked = (state & lvgl.STATE.CHECKED) ~= 0
    print("Checkbox state:", is_checked)
end, lvgl.EVENT.VALUE_CHANGED)

-- Make it interactive (already enabled by default for checkboxes)
cb:add_flag(lvgl.FLAG.CLICKABLE + lvgl.FLAG.CHECKABLE)

-- Check programmatically
cb:add_state(lvgl.STATE.CHECKED)   -- Check the checkbox
cb:clear_state(lvgl.STATE.CHECKED) -- Uncheck the checkbox

-- Check if checkbox is checked
local is_checked = (cb:get_state() & lvgl.STATE.CHECKED) ~= 0
```

### Dropdown

The Dropdown widget creates a list of options that can be selected from a dropdown menu.

```lua
-- Create a basic dropdown
local dd = parent:Dropdown {
    options = "Option 1\nOption 2\nOption 3",  -- Options separated by newlines
    align = lvgl.ALIGN.CENTER,                 -- Alignment within parent
}

-- Control methods
dd:open()                                      -- Open the dropdown list
dd:close()                                     -- Close the dropdown list

-- Add/remove options
dd:add_option("New Option", lvgl.DROPDOWN_POS_LAST)  -- Add option at position (use DROPDOWN_POS_LAST for end)
dd:clear_option()                                    -- Remove all options

-- Get information
local index = dd:option_index("Option 1")      -- Get index of option by text
local selected_text = dd:get("selected_str")   -- Get currently selected option text

-- Handle selection changes
dd:add_event(function(e)
    print("Selected:", dd:get("selected_str"))
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

The List widget is a container that helps create scrollable lists with text items and buttons. It's commonly used for creating menus, option lists, and other vertically arranged content.

```lua
-- Create a basic list
local list = parent:List {
    w = 200,                    -- Width
    h = 300,                    -- Height
    align = lvgl.ALIGN.CENTER   -- Alignment
}

-- Add text header
list:add_text("Categories")

-- Add buttons with icons
list:add_btn(lvgl.SYMBOL.HOME, "Home")
list:add_btn(lvgl.SYMBOL.FILE, "Documents")
list:add_btn(lvgl.SYMBOL.IMAGE, "Pictures")

-- Handle button clicks
list:add_event(function(e)
    if e:get_code() == lvgl.EVENT.CLICKED then
        local btn = e:get_target()
        local text = list:get_btn_text(btn)
        print("Clicked:", text)
    end
end, lvgl.EVENT.CLICKED)

-- Example: Create a styled list
local style_list = lvgl.Style()
style_list:set {
    bg_color = "#ffffff",      -- White background
    border_width = 1,          -- Add border
    border_color = "#888888",  -- Gray border
    pad_all = 4,              -- Inner padding
    radius = 8                -- Rounded corners
}

local style_btn = lvgl.Style()
style_btn:set {
    bg_color = "#eeeeee",     -- Light gray background
    bg_color_pressed = "#cccccc",  -- Darker when pressed
    pad_all = 10,             -- Button padding
    radius = 4,               -- Slightly rounded corners
    text_color = "#000000"    -- Black text
}

-- Apply styles
list:add_style(style_list, lvgl.STATE.DEFAULT)
list:add_style(style_btn, lvgl.STATE.DEFAULT, lvgl.PART.ITEMS)
```

### Roller

**WARNING**: Roller is broken, crashes

```lua
local roller = parent:Roller {
    options = "Option 1\nOption 2\nOption 3\nOption 4\nOption 5",  -- Options separated by newlines
    visible_row_count = 3,  -- Number of visible rows
    selected = 0  -- Index of initially selected option (0-based)
}

-- Get the selected option's text
local selected_text = roller:get_selected_str()

-- Get total number of options
local option_count = roller:get_options_cnt()

-- Change selection programmatically
roller:set { selected = 2 }  -- Select the third option (0-based index)

-- Handle selection changes
roller:add_event(function(e)
    print("Selected:", roller:get_selected())  -- Get selected index
    print("Selected text:", roller:get_selected_str())  -- Get selected text
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
-- Create an LED
local led = parent:Led()

-- Set its properties
led:set {
    color = "#FF0000",      -- Red color
    brightness = 150        -- Medium brightness
}

-- Control methods
led:on()                    -- Turn on (full brightness)
led:off()                   -- Turn off (minimum brightness)
led:toggle()                -- Toggle between on/off states
```

## Styling

### Creating Styles

Styles in LUAVGL are created using the `Style` constructor and can be applied to any object. A style can contain multiple properties that affect the appearance and behavior of an object.

```lua
local style = lvgl.Style()
style:set {
    -- style properties here
}
```

### Style Properties

LUAVGL provides a comprehensive set of style properties organized into the following categories:

#### Size and Position
- `width`: Width of the object
- `min_width`: Minimum width
- `max_width`: Maximum width
- `height`: Height of the object
- `min_height`: Minimum height
- `max_height`: Maximum height
- `x`: X position
- `y`: Y position
- `align`: Alignment type

#### Transform Properties
- `transform_width`: Width transformation
- `transform_height`: Height transformation
- `translate_x`: X translation
- `translate_y`: Y translation
- `transform_scale_x`: X scale transformation
- `transform_scale_y`: Y scale transformation
- `transform_rotation`: Rotation transformation
- `transform_pivot_x`: X pivot point for transformations
- `transform_pivot_y`: Y pivot point for transformations

#### Padding
- `pad_top`: Top padding
- `pad_bottom`: Bottom padding
- `pad_left`: Left padding
- `pad_right`: Right padding
- `pad_row`: Row padding
- `pad_column`: Column padding
- `pad_gap`: Gap padding
- `pad_all`: Sets all padding values (shorthand)
- `pad_ver`: Sets vertical padding (shorthand)
- `pad_hor`: Sets horizontal padding (shorthand)

#### Background
- `bg_color`: Background color
- `bg_opa`: Background opacity
- `bg_grad_color`: Gradient end color
- `bg_grad_dir`: Gradient direction
- `bg_main_stop`: Position of the main color in gradient
- `bg_grad_stop`: Position of the gradient end color
- `bg_image_src`: Background image source
- `bg_image_opa`: Background image opacity
- `bg_image_recolor`: Background image recolor
- `bg_image_recolor_opa`: Background image recolor opacity
- `bg_image_tiled`: Whether the background image is tiled

#### Border
- `border_color`: Border color
- `border_opa`: Border opacity
- `border_width`: Border width
- `border_side`: Which sides have borders
- `border_post`: Whether to draw border after content

#### Outline
- `outline_width`: Outline width
- `outline_color`: Outline color
- `outline_opa`: Outline opacity
- `outline_pad`: Space between object and outline

#### Shadow
- `shadow_width`: Shadow width
- `shadow_offset_x`: Shadow X offset
- `shadow_offset_y`: Shadow Y offset
- `shadow_spread`: Shadow spread
- `shadow_color`: Shadow color
- `shadow_opa`: Shadow opacity

#### Image
- `image_opa`: Image opacity
- `image_recolor`: Image recolor
- `image_recolor_opa`: Image recolor opacity

#### Line
- `line_width`: Line width
- `line_dash_width`: Width of dash
- `line_dash_gap`: Gap between dashes
- `line_rounded`: Whether line ends are rounded
- `line_color`: Line color
- `line_opa`: Line opacity

#### Arc
- `arc_width`: Arc width
- `arc_rounded`: Whether arc ends are rounded
- `arc_color`: Arc color
- `arc_opa`: Arc opacity
- `arc_image_src`: Image source for arc background

#### Text
- `text_color`: Text color
- `text_opa`: Text opacity
- `text_font`: Text font
- `text_letter_space`: Space between letters
- `text_line_space`: Space between lines
- `text_decor`: Text decoration
- `text_align`: Text alignment

#### Miscellaneous
- `radius`: Corner radius
- `clip_corner`: Whether to clip the corners
- `opa`: Overall opacity
- `color_filter_opa`: Color filter opacity
- `anim_time`: Animation time
- `blend_mode`: Blend mode
- `layout`: Layout type
- `base_dir`: Base direction for text

### Applying Styles

Styles can be applied to objects in several ways:

1. Direct style application:
```lua
obj:add_style(style, lvgl.STATE.DEFAULT)
```

2. Inline styling:
```lua
obj:set_style({
    bg_color = "#FF0000",
    border_width = 2
}, lvgl.STATE.DEFAULT)
```

3. State-specific styling:
```lua
-- Style for pressed state
obj:add_style(pressed_style, lvgl.STATE.PRESSED)

-- Style for disabled state
obj:add_style(disabled_style, lvgl.STATE.DISABLED)
```

### Style Inheritance

LUAVGL supports style inheritance using the special value "inherit":
```lua
obj:set_style({
    text_font = "inherit",
    text_color = "inherit"
}, lvgl.STATE.DEFAULT)
```

### Style Combinations

Some style properties can be set using shorthand combinations:

```lua
style:set {
    -- Set both width and height
    size = 100,
    
    -- Set all paddings at once
    pad_all = 10,
    
    -- Set vertical padding (top and bottom)
    pad_ver = 20,
    
    -- Set horizontal padding (left and right)
    pad_hor = 15
}
```

### Removing Styles

Styles can be removed from objects:
```lua
-- Remove a specific style
obj:remove_style(style)

-- Remove all styles
obj:remove_style_all()

-- Remove a specific property from a style
style:remove_prop("width")
```

### Example: Complex Styling

```lua
local style = lvgl.Style()
style:set {
    -- Size and position
    width = 200,
    height = 100,
    
    -- Background
    bg_color = "#2196F3",
    bg_opa = 255,
    bg_grad_color = "#1976D2",
    bg_grad_dir = lvgl.GRAD_DIR.HOR,
    
    -- Border
    border_width = 2,
    border_color = "#000000",
    border_opa = 128,
    
    -- Shadow
    shadow_width = 10,
    shadow_color = "#000000",
    shadow_opa = 64,
    shadow_offset_x = 5,
    shadow_offset_y = 5,
    
    -- Text
    text_color = "#FFFFFF",
    text_font = lvgl.BUILTIN_FONT.MONTSERRAT_14,
    text_letter_space = 1,
    text_line_space = 2,
    
    -- Padding
    pad_all = 10,
    
    -- Radius
    radius = 8
}

-- Apply the style to an object
obj:add_style(style, lvgl.STATE.DEFAULT)
```

This style creates a blue gradient button with a border, shadow, and specific text formatting.

## Layouts

### Flex Layout

The flex layout provides a powerful and flexible way to arrange child objects. It is based on the CSS Flexbox model.

#### Enabling Flex Layout

To use flex layout, set the layout property to `LAYOUT_FLEX`:

```lua
container:set({
    layout = lvgl.LAYOUT_FLEX
})
```

#### Flex Direction and Wrapping

Set the flex flow using `flex_flow` property or `set_flex_flow()` method. Available options:

- `lvgl.FLEX_FLOW.ROW`: Items arranged in a row (left to right)
- `lvgl.FLEX_FLOW.COLUMN`: Items arranged in a column (top to bottom)
- `lvgl.FLEX_FLOW.ROW_WRAP`: Items in rows, wrapping to next row when needed
- `lvgl.FLEX_FLOW.ROW_REVERSE`: Items in row from right to left
- `lvgl.FLEX_FLOW.ROW_WRAP_REVERSE`: Wrapped rows in reverse order
- `lvgl.FLEX_FLOW.COLUMN_WRAP`: Items in columns, wrapping to next column
- `lvgl.FLEX_FLOW.COLUMN_REVERSE`: Items in column from bottom to top
- `lvgl.FLEX_FLOW.COLUMN_WRAP_REVERSE`: Wrapped columns in reverse order

Example:
```lua
-- Using property
container:set({
    flex = {
        flex_direction = "row",
        flex_wrap = "wrap"
    }
})

-- Or using method
container:set_flex_flow(lvgl.FLEX_FLOW.ROW_WRAP)
```

#### Alignment

Flex layout supports three types of alignment:

1. **Main Axis Alignment** (`justify_content`):
   - `START`: Items at start of main axis
   - `END`: Items at end of main axis
   - `CENTER`: Items centered on main axis
   - `SPACE_BETWEEN`: Equal space between items
   - `SPACE_AROUND`: Equal space around items
   - `SPACE_EVENLY`: Equal space between and around items

2. **Cross Axis Alignment** (`align_items`):
   - `START`: Items at start of cross axis
   - `END`: Items at end of cross axis
   - `CENTER`: Items centered on cross axis
   - `SPACE_BETWEEN`: Equal space between items
   - `SPACE_AROUND`: Equal space around items
   - `SPACE_EVENLY`: Equal space between and around items

3. **Multi-line Alignment** (`align_content`):
   - Same options as above, but for multiple lines/columns

Example:
```lua
-- Using properties
container:set({
    flex = {
        justify_content = "space-between",
        align_items = "center",
        align_content = "space-around"
    }
})

-- Or using method
container:set_flex_align(
    lvgl.FLEX_ALIGN.SPACE_BETWEEN,  -- main axis
    lvgl.FLEX_ALIGN.CENTER,         -- cross axis
    lvgl.FLEX_ALIGN.SPACE_AROUND    -- track (multi-line)
)
```

#### Flex Grow

Control how items grow to fill available space using `flex_grow`:

```lua
-- Make an item grow to fill remaining space
item:set_flex_grow(1)

-- Make an item grow twice as much as others
item:set_flex_grow(2)
```

#### Example Layout

Here's a complete example creating a responsive flex layout:

```lua
local container = lvgl.obj(screen)
container:set({
    width = lvgl.HOR_RES(),
    height = lvgl.VER_RES(),
    layout = lvgl.LAYOUT_FLEX,
    flex = {
        flex_direction = "row",
        flex_wrap = "wrap",
        justify_content = "space-between",
        align_items = "center"
    }
})

-- Add some items
for i = 1, 5 do
    local item = lvgl.obj(container)
    item:set({
        width = 100,
        height = 100
    })
    if i == 1 then
        item:set_flex_grow(1)  -- First item grows to fill space
    end
end
```

This creates a responsive container with wrapped items, where the first item grows to fill available space while maintaining equal spacing between items.

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

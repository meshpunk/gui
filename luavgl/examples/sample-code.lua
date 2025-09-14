local analogTime = lvgl.AnalogTime {
    border_width = 1,
    border_color = "#F00",
    x = lvgl.HOR_RES() // 2,
    y = lvgl.VER_RES() // 2,
    hands = {
        hour = SCRIPT_PATH .. "/assets/hand-hour.png",
        minute = SCRIPT_PATH .. "/assets/hand-minute.png",
        second = SCRIPT_PATH .. "/assets/hand-second.png",
    },
    period = 33,
}

print("analogTime: ", analogTime)
local root = lvgl.Object()
root:set { w = lvgl.HOR_RES(), h = lvgl.VER_RES() }

-- create image on root and set position/img src/etc. properties.
root:Object():Image {
    src = SCRIPT_PATH .. "/assets/lvgl-logo.png",
    x = 0,
    y = 0,
    bg_color = 0x004400,
    pad_all = 0,
    align = lvgl.ALIGN.CENTER,
}:Anim {
    run = true,
    start_value = 0,
    end_value = 3600,
    duration = 2000,
    repeat_count = 2,
    path = "bounce",
    exec_cb = function(obj, value)
        obj:set {
            rotation = value
        }
    end,
    done_cb = function (anim, obj)
        print("anim done.: ", anim, "obj:", obj)
        anim:delete()
    end
}

-- second anim example with playback
local obj = root:Object {
    bg_color = "#F00000",
    radius = lvgl.RADIUS_CIRCLE,
    align = lvgl.ALIGN.LEFT_MID,
}
obj:clear_flag(lvgl.FLAG.SCROLLABLE)

--- @type AnimPara
local animPara = {
    run = true,
    start_value = 10,
    end_value = 50,
    duration = 1000,
    playback_delay = 100,
    playback_time = 500,
    repeat_count = lvgl.ANIM_REPEAT_INFINITE,
    path = "ease_in_out",
}

animPara.exec_cb = function(obj, value)
    obj:set { size = value }
end

obj:Anim(animPara)

animPara.end_value = 240
animPara.exec_cb = function(obj, value)
    obj:set { x = value }
end

obj:Anim(animPara)
local lvgl = require("lvgl")

Object {
    flex = {
        flex_direction = "row",
        flex_wrap = "wrap",
        justify_content = "center",
        align_items = "center",
        align_content = "center",
    },
    w = 400,
    h = 100,
    align = lvgl.ALIGN.CENTER,

    -- Button with label, inside a container
    Object {
        w = 150,
        h = lvgl.PCT(80),
        bg_color = "#aa0",

        Button {
            Label {
                text = string.format("BUTTON %d", 1),
                align = lvgl.ALIGN.CENTER
            }
        }:center()
    }:clear_flag(lvgl.FLAG.SCROLLABLE),

    -- Label inside a container
    Object {
        w = 150,
        h = lvgl.PCT(80),
        Label {
            text = string.format("label %d", 2)
        }:center()
    }:clear_flag(lvgl.FLAG.SCROLLABLE)
}
-- a dropdown on top left
local dd = lvgl.Dropdown {
    -- note there are two "Orange"
    options = "Apple\nBanana\nOrange\nCherry\nGrape\nRaspberry\nMelon\nOrange\nLemon\nNuts",
    symbol = "\xEF\x81\x94",
    dir = lvgl.DIR.RIGHT,
    selected_highlight = false,
    text = nil,
    align = {
        type = lvgl.ALIGN.TOP_LEFT,
        x_ofs = 20,
        y_ofs = 20,
    }
}
local list = dd:get("list")
print("get dropdown list: ", list)
list:set { text_font = lvgl.BUILTIN_FONT.MONTSERRAT_20 }

print("available options:", dd:get("options"))

local cnt = 0
dd:onevent(lvgl.EVENT.VALUE_CHANGED,
    ---comment
    ---@param obj Dropdown
    ---@param code ObjEventCode
    function(obj, code)
        obj:add_option(string.format("Option:%d", cnt), dd:get("option_count"))
        cnt = cnt + 1
        print(obj:get("selected_str"), "option_count" .. obj:get("option_count"))
    end
)

-- another dropdown on top left
local dd = lvgl.Dropdown {
    options = "apple\nBanana\nOrange\nCherry\nGrape\nRaspberry\nMelon\nOrange\nLemon\nNuts",
    symbol = "\xEF\x81\xB7",
    dir = lvgl.DIR.BOTTOM,
    text = "SetText",
    selected = 5,
    text_font = lvgl.BUILTIN_FONT.MONTSERRAT_20,
    align = {
        type = lvgl.ALIGN.BOTTOM_MID,
        x_ofs = 0,
        y_ofs = -20,
    }
}


dd:get("list"):set { text_font = lvgl.Font("montserrat", 24) }
dd:onevent(lvgl.EVENT.VALUE_CHANGED, function(obj, code)
    print(obj:get("selected_str") .. ":" .. obj:get("selected"),
        "dir:" .. obj:get("dir"),
        "option_index" .. dd:option_index(obj:get("selected_str")))
end)



lvgl.Timer {
    period = 1000,
    cb = function(t)
        t:delete()
        dd:open()
        print("now dd should be opened: " .. (dd.is_open and "open" or "closed"))
    end
}

lvgl.Timer {
    period = 2000,
    cb = function(t)
        t:delete()
        dd:close()
        print("now dd should be closed: " .. (dd.is_open and "open" or "closed"))
    end
}
local container = lvgl.Object {
    w = lvgl.HOR_RES(),
    h = lvgl.VER_RES(),
    bg_color = "#888",
    bg_opa = lvgl.OPA(100),
    border_width = 0,
    radius = 0,
    flex = {
        flex_direction = "row",
        flex_wrap = "wrap"
    }
}

-- change color directly using property
container.bg_color = lvgl.palette.main(lvgl.palette.BLUE_GREY)

print("created container", container)

local function createBtn(parent, name)
    local root = parent:Object {
        w = lvgl.SIZE_CONTENT,
        h = lvgl.SIZE_CONTENT,
        bg_color = "#ccc",
        bg_opa = lvgl.OPA(100),
        border_width = 0,
        radius = 10,
        pad_all = 20,
    }

    root:onClicked(function()
        container:delete()
        require(name)
    end)

    root:Label {
        text = name,
        text_color = "#333",
        align = lvgl.ALIGN.CENTER,
    }
end

createBtn(container, "keyboard")
createBtn(container, "animation")
createBtn(container, "luaC")
createBtn(container, "declarative")
createBtn(container, "pointer")
createBtn(container, "analogTime")
createBtn(container, "userdata")
createBtn(container, "flappyBird/flappyBird")
createBtn(container, "tests")
-- demo of external widget added to lvgl. See simulator/extension.c

local extension = lvgl.Extension {
    border_width = 1,
    w = lvgl.PCT(20),
    h = lvgl.PCT(20),
    align = lvgl.ALIGN.CENTER,
}

extension:onClicked(function ()
    print("clicked")
end)

print("extension: ", extension)
local lvgl = require("lvgl")

local root = lvgl.Object {
    flex = {
        flex_direction = "row",
        flex_wrap = "wrap",
        justify_content = "center",
        align_items = "center",
        align_content = "center",
    },
    w = 300,
    h = 75,
    align = lvgl.ALIGN.CENTER
}

for i = 1, 10 do
    local item = root:Object {
        w = 100,
        h = lvgl.PCT(100),
    }
    item:clear_flag(lvgl.FLAG.SCROLLABLE)

    local label = item:Label {
        text = string.format("label %d", i)
    }
    label:center()
end
local label1 = lvgl.Label {
    x = 0, y = 0,
    text_font = lvgl.Font("montserrat", 32, "normal"),
    text = "Hello Font",
    align = lvgl.ALIGN.CENTER
}

print("label1: ", label1)

lvgl.Label {
    -- x= 0, y = 0,
    -- text_font = lvgl.Font("MiSansW medium, montserrat", 24),
    text_font = lvgl.BUILTIN_FONT.MONTSERRAT_22,
    text = "Hello Font2",
}:align_to({
    type = lvgl.ALIGN.OUT_BOTTOM_LEFT,
    base = label1,
})
local function fs_example()
    -- local f <close>, msg, code = lvgl.open_file(SCRIPT_PATH .. "/tmp.txt") -- for lua 5.4
    local f, msg, code = lvgl.fs.open_file(SCRIPT_PATH .. "/tmp.txt", "rw")
    if not f then
        print("failed: ", msg, code)
        return
    end

    print("f: ", f)

    f:write("0123456789", 123, "\n", "the remaining text")
    f:seek("set", 0) -- go back
    local header, remaining = f:read(10, "*a")
    f:close()
    if not header then
        print("read failed or EOF")
        return
    end

    print("header len:", #header, ": ", header)
    print("remaining: ", remaining)

    lvgl.Label {
        x = 0,
        y = 0,
        text_font = lvgl.Font("montserrat", 20, "normal"),
        text = header .. remaining,
        align = lvgl.ALIGN.TOP_LEFT
    }

    local list = lvgl.List {
        align = lvgl.ALIGN.TOP_RIGHT,
        pad_all = 10,
        text_font = lvgl.BUILTIN_FONT.MONTSERRAT_12
    }
    list:add_text("Directory list:")

    -- local dir <close>, msg, code = lvgl.fs.open_dir(SCRIPT_PATH .. "/")
    local dir, msg, code = lvgl.fs.open_dir(SCRIPT_PATH .. "/")
    if not dir then
        print("open dir failed: ", msg, code)
        return
    end

    while true do
        local d = dir:read()
        if not d then break end
        local is_dir = string.byte(d, 1) == string.byte("/", 1)
        local str = (is_dir and "dir: " or "file: ") .. d
        print(str)

        list:add_text(str):set{
            border_width = 1,
        }
    end
    dir:close()
end

fs_example()

local scr = lvgl.disp.get_scr_act()
local indev = lvgl.indev.get_act()
print("diplay screen_act:", scr, "indev:", indev)

scr:onevent(lvgl.EVENT.GESTURE, function (obj, code)
    print("gesture dir: ", indev:get_gesture_dir())
end)

scr:onevent(lvgl.EVENT.CLICKED, function (obj, code)
    print("clicked: " .. code)
end)
local function group_example()
    local g = lvgl.group.create()
    g:set_default()

    -- for demo purpose, set all indev to use this group
    local indev = nil
    while true do
        indev = lvgl.indev.get_next(indev)
        if not indev then break end

        local t = indev:get_type()
        if t == 2 or t == 4 then
            indev:set_group(g)
        end
    end

    local style = lvgl.Style({
            border_width = 5,
            border_color = "#a00",
        })

    local root = lvgl.Object {
            w = lvgl.PCT(100),
            h = lvgl.PCT(100),
            align = lvgl.ALIGN.CENTER,
            bg_color = "#aaa",
            flex = {
                flex_direction = "row",
                flex_wrap = "wrap"
            }
        }

    root:add_style(style, lvgl.STATE.FOCUSED)

    for _ = 1, 5 do
        local obj = root:Object({
                w = lvgl.PCT(50),
                h = lvgl.PCT(50),
                bg_color = "#555",
            })

        obj:add_style(style, lvgl.STATE.FOCUSED)

        obj:onClicked(function(obj, code)
            print("clicked: ", obj)
        end)

        obj:onevent(lvgl.EVENT.FOCUSED, function(obj, code)
            print("focused: ", obj)
            obj:scroll_to_view(true)
        end)

        obj:onevent(lvgl.EVENT.DEFOCUSED, function(obj, code)
            print("defocused: ", obj)
        end)

        g:add_obj(obj)
    end
end

group_example()
local function indev_example()
    local indev = lvgl.indev.get_act()
    print("act indev: ", indev)


    indev:on_event(lvgl.EVENT.SHORT_CLICKED, function ()
        print("indev pressed")
    end)
    local obj_act = lvgl.indev.get_obj_act()
    print("obj_act: ", obj_act)

    local root = lvgl.Object {
            w = lvgl.PCT(30),
            h = lvgl.PCT(30),
            align = lvgl.ALIGN.CENTER,
            bg_color = "#aaa",
        }

    root:Object({
        w = 1000,
        h = 1000,
        align = lvgl.ALIGN.CENTER,
        bg_color = "#555",
    }):onevent(lvgl.EVENT.ALL, function(obj, code)
        local indev = lvgl.indev.get_act()

        if not indev then return end

        if code == lvgl.EVENT.PRESSED then
            local x, y = indev:get_point()
            print("pressed: ", x, y)
        end

        if code == lvgl.EVENT.PRESSING then
            local x, y = indev:get_vect()
            print("vect: ", x, y)
        end

        if code == lvgl.EVENT.GESTURE then
            local gesture_dir = indev:get_gesture_dir()
            print("gesture_dir: ", gesture_dir)
        end

        if code == lvgl.EVENT.RELEASED then
            local scroll_dir = indev:get_scroll_dir()
            print("scroll_dir: ", scroll_dir)

            local scroll_obj = indev:get_scroll_obj()
            print("scroll_obj: ", scroll_obj)

            local x, y = indev:get_vect()
            local obj_search = obj:indev_search(x, y)
            print("indev search obj: ", obj_search)

            local obj_search = obj:indev_search({x, y})
            print("indev search obj2: ", obj_search)
        end
    end)
end

indev_example()
local root = lvgl.Object {
    w = lvgl.HOR_RES(),
    h = lvgl.VER_RES(),
    pad_all = 0,
    bg_color = "#333",
    bg_opa = lvgl.OPA(50),
}
root:add_flag(lvgl.FLAG.CLICKABLE)

local ta = root:Textarea {
    password_mode = false,
    one_line = true,
    text_font = lvgl.Font("montserrat", 22),
    text_color = "#FFF",
    text = "Input text here",
    w = lvgl.SIZE_CONTENT,
    h = lvgl.SIZE_CONTENT,
    bg_opa = 0,
    border_width = 2,
    pad_all = 2,
    align = lvgl.ALIGN.TOP_MID,
}

print("created textarea: ", ta)

local keyboard = root:Keyboard {
    textarea = ta,
    align = lvgl.ALIGN.BOTTOM_MID,
}
print("created keyboard: ", keyboard)

ta:onevent(lvgl.EVENT.PRESSED, function(obj, code)
    keyboard:clear_flag(lvgl.FLAG.HIDDEN)
end)

ta:onevent(lvgl.EVENT.READY, function()
    keyboard:add_flag(lvgl.FLAG.HIDDEN)
end)

ta:onevent(lvgl.EVENT.CANCEL, function(obj, code)
    keyboard:add_flag(lvgl.FLAG.HIDDEN)
end)

root:onClicked(function (obj, code)
    keyboard:add_flag(lvgl.FLAG.HIDDEN)
end)
-- Everything is already setup in C code, check lua_in_c.c
-- This file is just to trigger the demo

embed_lua_in_c()
local lvgl = require("lvgl")

local container = lvgl.Label {
    w = lvgl.HOR_RES(),
    h = lvgl.VER_RES(),
    bg_color = "#888",
    bg_opa = lvgl.OPA(100),
    border_width = 0,
    radius = 0,
    flex = {
        flex_direction = "row",
        flex_wrap = "wrap"
    }
}

local obj1 = container:Object()
obj1.id = "Obj1"

local obj2 = obj1:Object()
obj2.id = "child"

local obj3 = container:Object()
obj3.id = "Obj3"

local obj4 = obj3:Object()
obj4.id = "child"

local failed = false
local obj = lvgl.get_child_by_id("Obj1") -- obj1
failed = failed or obj ~= obj1
print("obj id", obj.id)

local obj = obj1:get_child_by_id("child") -- obj1
print("obj id", obj.id, obj == obj2)
failed = failed or obj ~= obj2


local obj = obj3:get_child_by_id("child") -- obj1
print("obj id", obj.id, obj == obj4)
failed = failed or obj ~= obj4

lvgl.Label{
    text_color = "#333",
    align = lvgl.ALIGN.CENTER,
    text = "Test " .. (failed and "Failed" or "Success")
}local root = lvgl.Object {
    w = 1,
    h = 0,
    pad_all = 0,
    align = lvgl.ALIGN.CENTER,
    border_width = 1,
    border_color = "#F00",
    flag_scrollable = false,
}

local pointer = root:Pointer {
    src = SCRIPT_PATH .. "/assets/second.png",
    pivot = { 20, 233 },
    x = -20,
    y = -233,
    border_width = 1,
    border_color = "#0F0",
    range = {
        valueStart = 0,
        valueRange = 60000,
        angleStart = 0,
        angleRange = -3600
    },
}

print("img w,h: ", pointer:get_img_size())

pointer:Anim{
    run = true,
    start_value = 0,
    end_value = 60000,
    duration = 60000,
    path = "linear",
    repeat_count = lvgl.ANIM_REPEAT_INFINITE,
    exec_cb = function(obj, value)
        obj:set {
            value = value
        }
    end,
}

print("created pointer: ", pointer)

local function testCrash()
    print(nil .. "")
end

local function testCrash2()
    testCrash()
end
local function testCrash3()
    testCrash2()
end
local function testCrash4()
    testCrash3()
end

local function testCrash5()
    testCrash4()
end

local function testCrash6()
    testCrash5()
end

lvgl.Label {
    text = "crash in 1 second.",
    align = lvgl.ALIGN.CENTER
}

lvgl.Timer {
    period = 1000,
    cb = function(t)
        t:delete()
        -- crash in callback function is protected.
        testCrash6()
    end
}local roller = lvgl.Roller {
    options = {
        options = "January\nFebruary\nMarch\nApril\nMay\nJune\nJuly\nAugust\nSeptember\nOctober\nNovember\nDecember",
        mode = lvgl.ROLLER_MODE.INFINITE,
    },
    visible_row_count = 4,
    align = lvgl.ALIGN.CENTER
}

print("create roller:", roller)
-- roller:onevent(lvgl.EVENT.VALUE_CHANGED, function (obj, code)
--     print(obj:get_selected_str())
-- end)
local function Object(t)
    local info = debug.getinfo(2, "nS") -- 2 refers to the caller of the function
    print("Called from: " .. info.short_src .. ", function " .. (info.name or 'unknown'))
    print("table content:")
    obj = {table.unpack(t)}
    for k, v in pairs(t) do
        print(k, v)
    end
    return obj
end

Object({
    a = "789",
    Object({
        a = "101112",
    })
})
local container = lvgl.Object {
    w = lvgl.HOR_RES(),
    h = lvgl.VER_RES(),
    bg_color = "#888",
    bg_opa = lvgl.OPA(100),
    border_width = 0,
    radius = 0,
    flex = {
        flex_direction = "row",
        flex_wrap = "wrap"
    }
}

local function createBtn(parent, name)
    local root = parent:Object {
        w = lvgl.SIZE_CONTENT,
        h = lvgl.SIZE_CONTENT,
        bg_color = "#ccc",
        bg_opa = lvgl.OPA(100),
        border_width = 0,
        radius = 10,
        pad_all = 20,
    }

    root:onClicked(function()
        container:delete()
        require(name)
    end)

    root:Label {
        text = name,
        text_color = "#333",
        align = lvgl.ALIGN.CENTER,
    }
end

createBtn(container, "gesture")
createBtn(container, "objid")
createBtn(container, "font")
createBtn(container, "uservalue")
createBtn(container, "roller")
createBtn(container, "dropdown")
createBtn(container, "extension")
createBtn(container, "fs")
createBtn(container, "indev")
createBtn(container, "group")
createBtn(container, "protectedcall")
-- user_data can store any type of lua value
local obj1 = lvgl.Label {
    text_color = "#333",
    border_color = "#f00",
    border_width = 1,
    align = lvgl.ALIGN.TOP_LEFT
}

obj1.user_data = 123.456
obj1.text = "user_data:\n" .. obj1.user_data

-- use user_data to store table
local obj2 = lvgl.Label {
    text_color = "#333",
    border_color = "#f00",
    border_width = 1,
    align = lvgl.ALIGN.TOP_RIGHT
}

obj2.user_data = {
    abc = "hello",
    def = "world",
    [1] = 123,
}
obj2.text = "user_data:"
for k, v in pairs(obj2.user_data) do
    print(k, v)
    obj2.text = obj2.text .. string.format("%s: %s\n", k, v)
end

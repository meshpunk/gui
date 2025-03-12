-- Main messenger application

-- Helper function to create buttons
local function createBtn(parent, name)
    local root = parent:Button {
        w = lvgl.SIZE_CONTENT,
        h = lvgl.SIZE_CONTENT,
    }

    root:onClicked(function()
        print("Button clicked!")
        root:set { bg_color = "#00FF00" }
    end)

    root:Label {
        text = name,
        text_color = "#333",
        align = lvgl.ALIGN.CENTER,
    }
    
    return root
end

-- Create a simple hello world label
local root = lvgl.Object()
root:set { w = lvgl.HOR_RES(), h = lvgl.VER_RES() }

-- flex layout and align
root:set {
    flex = {
        flex_direction = "column",
        flex_wrap = "wrap",
        justify_content = "center",
        align_items = "center",
        align_content = "center",
    },
    w = 320,
    h = 240,
    align = lvgl.ALIGN.CENTER
}

label = root:Label {
    text = string.format("Messenger App"),
    text_font = lvgl.BUILTIN_FONT.MONTSERRAT_28,
    align = lvgl.ALIGN.CENTER
}        

local btn = createBtn(root, "Send Message")

-- Create textarea
local ta = root:Textarea {
    password_mode = false,
    one_line = true,
    text = "Type a message...",
    w = 280,
    h = 40,
    pad_all = 2,
    align = lvgl.ALIGN.TOP_MID,
}

print("created textarea: ", ta)

-- Animation example with playback
local obj = root:Object {
    bg_color = "#F00000",
    radius = lvgl.RADIUS_CIRCLE,
    size = 24,
    x = 280,
    y = 200
}
obj:clear_flag(lvgl.FLAG.SCROLLABLE)

-- Animation parameters
local animPara = {
    run = true,
    start_value = 16,
    end_value = 32,
    duration = 1000,
    repeat_count = lvgl.ANIM_REPEAT_INFINITE,
    path = "ease_in_out",
}

animPara.exec_cb = function(obj, value)
    obj:set { size = value }
end

obj:Anim(animPara)

-- Return the root object
return root
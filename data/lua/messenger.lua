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
    w = lvgl.HOR_RES(),
    h = lvgl.VER_RES(),
    align = lvgl.ALIGN.TOP_LEFT
}

label = root:Label {
    text = string.format("Messenger App"),
    text_font = lvgl.BUILTIN_FONT.MONTSERRAT_28,
    align = lvgl.ALIGN.CENTER
}        

local form = root:Object {
    flex = {
        flex_direction = "row",
        align = lvgl.ALIGN.BOTTOM_MID,
        x_ofs = 0,
        y_ofs = -150,
        justify = "space_between" -- Spread elements across available space
    },
    border_width = 0,
    w = lvgl.HOR_RES(),
    pad_all = 0, -- Add some padding for aesthetics

}

local ta = form:Textarea {
    password_mode = false,
    one_line = true,
    text = "Type a message...",
    w = lvgl.PCT(80), -- 80% of parent width
    h = 40,
    align = lvgl.ALIGN.LEFT_MID
}

local btn = createBtn(form, "Send")


-- Display animation
if false then
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
end
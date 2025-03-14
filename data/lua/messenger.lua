-- Main messenger application

local messages = {
    {
        text = "Yo whats the best folk punk band",
        sender = "0x1234567890",
        timestamp = "12:00 PM"
    },
    {
        text = "This bike is a pipe bomb",
        sender = "0x9876543210",
        timestamp = "12:01 PM"
    }
}

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
root:clear_flag(lvgl.FLAG.SCROLLABLE)

-- flex layout and align
root:set {
    w = lvgl.HOR_RES(),
    h = lvgl.VER_RES(),
    align = lvgl.ALIGN.TOP_LEFT,
    pad_all = 0,
    border_width = 0,
}

-- label = root:Label {
--     text = string.format("Messenger App"),
--     text_font = lvgl.BUILTIN_FONT.MONTSERRAT_28,
--     align = lvgl.ALIGN.CENTER
-- }        

local message_view = root:Object {
    flex = {
        flex_direction = "column",
        flex_wrap = "nowrap",
        align = lvgl.ALIGN.TOP_LEFT,
    },
    border_width = 0,
    h = lvgl.PCT(100),
    w = lvgl.PCT(100),
    pad_all = 0
}

function update_message_list()
    -- label:delete()
    -- message_view:clear()

    for _, message in ipairs(messages) do
        local message_item = message_view:Object {
            flex = {
                flex_direction = "row",
                justify = "space_between"
            },
            w = lvgl.PCT(100),
            h = lvgl.SIZE_CONTENT,
            border_width = 0,
        }
        message_item:clear_flag(lvgl.FLAG.SCROLLABLE)

        message_item:Label {
            text = message.text,
            h = lvgl.SIZE_CONTENT,
            w = lvgl.PCT(100),

        }   
    end
end

update_message_list()

local form = root:Object {
    flex = {
        flex_direction = "row",
        justify = "space_between"
    },
    border_width = 0,
    y = lvgl.VER_RES() - 55,
    w = lvgl.HOR_RES(),
    h = 40,
    pad_bottom = 0,
    pad_top = 0,
    pad_left = 10,
}

form:clear_flag(lvgl.FLAG.SCROLLABLE)

local ta = form:Textarea {
    password_mode = false,
    one_line = true,
    placeholder = "Type a message...",
    w = lvgl.PCT(75),
    h = 40,
    align = lvgl.ALIGN.LEFT_MID,
}

ta:onevent(lvgl.EVENT.KEY, function(obj, code)
    local indev = lvgl.indev.get_act()
    local key = indev:get_key()

    print("key pressed")
    print(key)

    if key == lvgl.KEY.ENTER then
        messages[#messages + 1] = {
            text = ta.text,
            sender = "0x1234567890",
            timestamp = "12:00 PM"
        }

        update_message_list()

        ta.text = ""
    end
end)

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
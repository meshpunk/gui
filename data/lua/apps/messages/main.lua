local messages = {
    {
        text = "Hello, how are you?",
        sender = "Max",
        timestamp = "12:00 PM"
    },
    {
        text = "I'm good, thanks!",
        sender = "Jane",
        timestamp = "12:01 PM"
    },
    {
        text = "What's up?",
        sender = "Max",
        timestamp = "12:02 PM"
    },
    {
        text = "Not much, just chilling",
        sender = "Jane",
        timestamp = "12:03 PM"
    },
    {
        text = "What's your name?",
        sender = "Max",
        timestamp = "12:04 PM"
    },
    {
        text = "I'm John",
        sender = "Jane",
        timestamp = "12:05 PM"
    },
    {
        text = "What's your favorite color?",
        sender = "Max",
        timestamp = "12:06 PM"
    },
    {
        text = "I like blue",
        sender = "Jane",
        timestamp = "12:07 PM"
    },
    {
        text = "What's your favorite food?",
        sender = "Max",
        timestamp = "12:08 PM"
    },
    {
        text = "I like pizza",
        sender = "Jane",
        timestamp = "12:09 PM"
    },
    {   
        text = "What's your favorite animal?",
        sender = "Max",
        timestamp = "12:10 PM"
    },
    {
        text = "I like dogs",
        sender = "Jane",
        timestamp = "12:11 PM"
    },
    {
        text = "What's your favorite number?",
        sender = "Max",
        timestamp = "12:12 PM"
    },
    {
        text = "I like 7",
        sender = "Jane",
        timestamp = "12:13 PM"
    }
}


-- root object
local root = lvgl.Object()
root:set { 
    w = lvgl.HOR_RES(), 
    h = lvgl.VER_RES(),
    border_width = 0,
    radius = 0,
    pad_all = 10,
    bg_color = "#aaaaaa",
    flex = {
        flex_direction = "column",
        flex_wrap = "nowrap",
        align = lvgl.ALIGN.TOP_LEFT,
    }
}

root:Label {
    text = "Messages",
    align = lvgl.ALIGN.CENTER,
}

root:Object {
    w = 16,
    h = 16,
    bg_color = "#000000",
    radius = 0,
    border_width = 0,
    pad_all = 0,
}

for i, message in ipairs(messages) do
    local right_align = message.sender == "Max"

    local message_box = root:Object {
        h = lvgl.SIZE_CONTENT,
        w = lvgl.PCT(100),
        border_width = 0,
        radius = 0,
        pad_all = 0,
        bg_color = "#aaaaaa",
        flex = {
            flex_direction = right_align and "row-reverse" or "row",
            flex_wrap = "nowrap",
            justify_content = right_align and "flex-end" or "flex-start",
            align_items = right_align and "flex-end" or "flex-start",
        }
    }:clear_flag(lvgl.FLAG.SCROLLABLE)
    
    -- sender "avatar"
    message_box:Object {
        size = lvgl.SIZE_CONTENT,
        bg_color = "#000000",
        border_width = 0,
        radius = 2,
        pad_hor = 2,
        pad_ver = 0,
    }:Label {
        text = message.sender:sub(1, 1):upper(),
        text_color = "#ffffff",
    }

    -- message
    message_box:Label {
        text = message.text,
        -- text_align = right_align and lvgl.ALIGN.LEFT_MID or lvgl.ALIGN.RIGHT_MID,
        -- align = right_align and lvgl.ALIGN.LEFT_MID or lvgl.ALIGN.RIGHT_MID,
    }
end

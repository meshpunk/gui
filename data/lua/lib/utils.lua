-- Utility functions for Lua apps

local utils = {}

-- Format time string
function utils.formatTime(timestamp)
    local time = os.date("*t", timestamp or os.time())
    return string.format("%02d:%02d", time.hour, time.min)
end

-- Create a simple notification
function utils.createNotification(parent, message, duration)
    duration = duration or 3000  -- default 3 seconds
    
    local notification = parent:Object {
        bg_color = "#333333", 
        radius = 8,
        border_width = 0,
        pad_all = 10,
        w = 280,
        h = lvgl.SIZE_CONTENT,
        align = lvgl.ALIGN.BOTTOM_MID,
        y = -20
    }
    
    notification:Label {
        text = message,
        text_color = "#FFFFFF",
        align = lvgl.ALIGN.CENTER,
    }
    
    -- Animate in from bottom
    notification:set { y = 50 }
    notification:Anim {
        run = true,
        start_value = 50,
        end_value = -20,
        duration = 300,
        path = "ease_out",
        exec_cb = function(obj, value)
            obj:set { y = value }
        end
    }
    
    -- Auto-destroy after duration
    notification.timer = lvgl.Timer.create(function()
        notification:delete()
    end, duration, 1)
    
    return notification
end

-- Return the module
return utils
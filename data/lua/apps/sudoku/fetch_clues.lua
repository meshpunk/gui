local wifi = require("wifi")
local json = require("json")

-- Coroutines for async operations

local ssid = "takiwa"
local password = "kupuhuna"

function parse_clues(fetched, difficulty_label)
    local parsed = json.parse(fetched)
    print("Difficulty: " .. parsed.newboard.grids[1].difficulty)
    fetched = parsed.newboard.grids[1].value
    local clues = {}
    for _, row in ipairs(fetched) do
        for _, cell in ipairs(row) do
            table.insert(clues, cell ~= 0 and cell or ".")
        end
    end
    difficulty_label.text = parsed.newboard.grids[1].difficulty
    return table.concat(clues)
end

function fetch_clues(callback, difficulty_label)
    local fetch_co = wifi.fetch("https://sudoku-api.vercel.app/api/dosuku")

    timer = lvgl.Timer {
        period = 100,
        cb = function(t)
            if not fetch_co then return end
            local success, result = wifi.resume(fetch_co)
        
            if not success then 
                error("Error fetching clues: " .. (result.error or "Unknown error"))
            elseif result and result.success then
                wifi.disconnect()
                fetch_co = nil
                timer:delete()
                timer = nil
    
                callback(parse_clues(result.body, difficulty_label))
            end
        end
    }
end

function connect_wifi(result_callback, difficulty_label)
    local connect_co = wifi.connect(ssid, password)

    timer = lvgl.Timer {
        period = 100,
        cb = function(t)
            if not connect_co then return end
    
            local success, result = wifi.resume(connect_co)
            
            if not success then
                connect_co = nil
                error("Error connecting to WiFi: " .. tostring(result))
            end
            
            if result == true then
                connect_co = nil
                fetch_clues(result_callback, difficulty_label)
            end
        end
    }
end

return function(result_callback, difficulty_label)
    print("fetch_clues")
    assert (not wifi.isConnected(), "WiFi already connected")
    connect_wifi(result_callback, difficulty_label)
end
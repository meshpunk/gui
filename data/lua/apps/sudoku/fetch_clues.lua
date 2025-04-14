local wifi = require("wifi")
local json = require("json")

-- Coroutines for async operations

local ssid = "takiwa"
local password = "kupuhuna"

function parse_clues(fetched)
    local parsed = json.parse(fetched)
    print("Difficulty: " .. parsed.newboard.grids[1].difficulty)
    fetched = parsed.newboard.grids[1].value
    local clues = {}
    for _, row in ipairs(fetched) do
        for _, cell in ipairs(row) do
            table.insert(clues, cell ~= 0 and cell or ".")
        end
    end
    return table.concat(clues)
end

return function(callback)
    print("fetch_clues")
    local connect_co = wifi.connect(ssid, password)
    local fetch_co
    local clues
    local timer

    timer = lvgl.Timer {
        period = 100,
        cb = function(t)
            print("Timer callback")
            if connect_co then
                local success, result = wifi.resume(connect_co)
                
                if not success then
                    print("Error: " .. tostring(result))
                    connect_co = nil
                    timer:delete()
                    timer = nil
                    return
                end
                
                if result == true then
                    connect_co = nil
                    fetch_co = wifi.fetch("https://sudoku-api.vercel.app/api/dosuku")
                end
            elseif fetch_co then
                local success, result = wifi.resume(fetch_co)
                -- Fetch completed or error
                if not success or result then
                    if success and result.success then clues = result.body
                    else print("Fetch error: " .. result.error) end
                    fetch_co = nil
                end
            else 
                if (wifi.isConnected()) then wifi.disconnect() end
                timer:pause()
                timer:delete()

                local clues = parse_clues(clues)
                callback(clues)
            end
        end
    }
end
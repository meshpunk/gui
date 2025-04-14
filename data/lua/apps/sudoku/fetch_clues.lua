local wifi = require("wifi")

-- Coroutines for async operations

local ssid = "takiwa"
local password = "kupuhuna"

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

                print("clues " .. clues)

                callback(".5.21.74...4...8......6...1.3.62......7...6......59.3.7...8......6...3...42.31.5.")
                -- callback(clues or ".5.21.74...4...8......6...1.3.62......7...6......59.3.7...8......6...3...42.31.5.")
            end
        end
    }
end
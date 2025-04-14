-- WiFi module for Lua apps

local wifi = {}

-- Internal state
local connected = false
local ssid = ""
local ip = ""

-- Connect to WiFi network
function wifi.connect(network, password)
  return coroutine.create(function()
    print("Connecting to WiFi: " .. network)
    _wifi_connect(network, password)
    
    -- Wait for connection
    local timeout = 5 -- 20 seconds timeout
    local start = os.time()
    
    while not connected and os.time() - start < timeout do
      local status, ip_addr, connected_ssid = _wifi_status()
      if status == "connected" then
        connected = true
        ssid = connected_ssid
        ip = ip_addr
        print("Connected to " .. ssid .. " with IP: " .. ip)
        coroutine.yield(true)
        return
      end
      coroutine.yield(false)
    end
    
    -- Timeout reached
    print("Failed to connect to WiFi")
    coroutine.yield(false)
  end)
end

-- Disconnect from WiFi
function wifi.disconnect()
  _wifi_disconnect()
  connected = false
  ssid = ""
  ip = ""
  return true
end

-- Get current WiFi status
function wifi.status()
  local status, ip_addr, connected_ssid = _wifi_status()
  return {
    connected = (status == "connected"),
    status = status,
    ssid = connected_ssid,
    ip = ip_addr
  }
end

-- Check if WiFi is connected
function wifi.isConnected()
  return connected
end

-- Fetch data from URL
function wifi.fetch(url, options)
  options = options or {}
  
  return coroutine.create(function()
    if not connected then
      print("WiFi not connected")
      coroutine.yield({ success = false, error = "WiFi not connected" })
      return
    end
    
    print("Fetching: " .. url)
    local result = _wifi_fetch(url, options.method or "GET", options.headers or {}, options.body or "")
    
    if not result.success then
      print("Fetch failed: " .. (result.error or "Unknown error"))
      coroutine.yield(result)
      return
    end
    
    print("Fetch successful: " .. result.status)
    coroutine.yield(result)
  end)
end

-- Resume a coroutine and check status
function wifi.resume(co)
  if coroutine.status(co) == "dead" then
    return false, "Coroutine is dead"
  end
  
  local success, result = coroutine.resume(co)
  
  if not success then
    return false, "Error: " .. tostring(result)
  end
  
  return true, result
end

-- Return the module
return wifi
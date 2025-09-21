local M = {
    __onMessage = nil,
    __history = {}
}

function M:onMessage(cb)
    M.__onMessage = cb
end

function M:broadcast(text)
    print("Broadcasting message:", text)
    
    -- Call the C++ function to actually send the message
    local success = _mesh_broadcast(text)
    
    if success then
        print("Message sent successfully")
        M.__dispatch(text, os.time(), true, 0)
        -- Note: We don't add to history here since it will come back
        -- through the mesh network and be received normally
    else
        print("Failed to send message")
    end
    
    return success
end

function M.__dispatch(text, timestamp, direct, hops)
    local msg = {
        from = "fixme", -- TODO: extract sender from MeshCore
        text = text,
        timestamp = timestamp,
        direct = direct,
        hops = hops
    }

    table.insert(M.__history, msg)

    if M.__onMessage then
        M.__onMessage(msg)
    end
end

function M:all()
    return self.__history
end

return M
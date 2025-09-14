local M = {
    __onMessage = nil,
    __history = {}
}

function M:onMessage(cb)
    M.__onMessage = cb
end

function M:broadcast(text)
    print("Broadcasting message:", text)
    
    -- Here you would add the actual broadcasting logic
    -- For now, we simulate receiving the message ourselves
    M.__dispatch(text, os.time(), true, 0)
end

function M.__dispatch(text, timestamp, direct, hops)
  local msg = {
    from = "fixme", -- maybe extract from text
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
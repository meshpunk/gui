local M = {
    __onMessage = nil,
    __history = {}
}

function M:onMessage(cb)
    M.__onMessage = cb
end

function M:broadcast(message)
    print(message)

    local from = "none"
    
    -- Here you would add the actual broadcasting logic
    -- For now, we simulate receiving the message ourselves
    M.__dispatch(message, os.time(), true, 0, from)
end

function M.__dispatch(text, timestamp, direct, hops, from)
  local msg = {
    from = from or "fixme",
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
local M = {
    __handlers = {},
    __history = {}
}

function M:on(event, cb)
    if event == "recv" then
        table.insert(self.__handlers, cb)
    end
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
  
  for _, cb in ipairs(M.__handlers) do
    cb(msg)
  end
end

function M:all()
    return self.__history
end

return M
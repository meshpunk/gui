local M = {}

-- obj: lvgl object
-- state: lvgl state
-- add: boolean, whether to add or clear the state
function M.propagate_state(obj, state, add)
    local fn = add and "add_state" or "clear_state"
    obj[fn](obj, state)
    for i = 0, obj:get_child_cnt() - 1 do M.propagate_state(obj:get_child(i), state, add) end
end

return M
local function print_pos(event)
    print(string.format("  pos: (%d, %d)  abs: (%d, %d)",
        event.x, event.y, event.x_abs, event.y_abs))
    print(string.format("  time: %d", event.time))
end

local function print_mods(mods)
    local held = {}
    for k, v in pairs(mods) do
        if v then held[#held + 1] = k end
    end
    if #held > 0 then
        print("  mods: " .. table.concat(held, ", "))
    end
end

function conky_on_mouse_event(event)
    if event.type == "button_down" then
        print("[button_down]")
        print_pos(event)
        print("  button: " .. tostring(event.button)
            .. " (code " .. tostring(event.button_code) .. ")")
        print_mods(event.mods)
        return true

    elseif event.type == "button_up" then
        print("[button_up]")
        print_pos(event)
        print("  button: " .. tostring(event.button)
            .. " (code " .. tostring(event.button_code) .. ")")
        print_mods(event.mods)
        return true

    elseif event.type == "mouse_scroll" then
        print("[mouse_scroll]")
        print_pos(event)
        print("  direction: " .. tostring(event.direction))
        print_mods(event.mods)
        return true

    elseif event.type == "mouse_move" then
        -- Uncomment to log movement (will spam the terminal):
        -- print(string.format("[mouse_move] (%d, %d)", event.x, event.y))
        return false

    elseif event.type == "mouse_enter" then
        print("[mouse_enter]")
        print_pos(event)

    elseif event.type == "mouse_leave" then
        print("[mouse_leave]")
        print_pos(event)
    end

    return false
end

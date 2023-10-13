local P = {}
op = P

function P:ordered_pairs(t)
  local i = {}
  for k in next, t do
    table.insert(i, k)
  end
  table.sort(i)
  return function()
    local k = table.remove(i)
    if k ~= nil then
      return k, t[k]
    end
  end
end

return op

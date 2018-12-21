-- tolua: enumerate class
-- Written by Waldemar Celes
-- TeCGraf/PUC-Rio
-- Jul 1998
-- $Id: enumerate.lua,v 1.3 2000/01/24 20:41:15 celes Exp $

-- This code is free software; you can redistribute it and/or modify it.
-- The software provided hereunder is on an "as is" basis, and
-- the author has no obligation to provide maintenance, support, updates,
-- enhancements, or modifications.


-- Enumerate class
-- Represents enumeration
-- The following fields are stored:
--    {i} = list of constant names
classEnumerate = {
}
classEnumerate.__index = classEnumerate
setmetatable(classEnumerate,classFeature)

-- register enumeration
function classEnumerate:register (pre)
	if not self:check_public_access() then
		return
	end
 pre = pre or ''
 local nspace = getnamespace(classContainer.curr)
 local i=1
 while self[i] do
 	if self.lnames[i] and self.lnames[i] ~= "" then
	
		output(pre..'tolua_constant(tolua_S,"'..self.lnames[i]..'",'..nspace..self[i]..');')
	end
  i = i+1
 end
end

-- Print method
function classEnumerate:print (ident,close)
 print(ident.."Enumerate{")
 print(ident.." name = "..self.name)
 local i=1
 while self[i] do
  print(ident.." '"..self[i].."'("..self.lnames[i].."),")
  i = i+1
 end
 print(ident.."}"..close)
end

-- Internal constructor
function _Enumerate (t,varname)
 setmetatable(t,classEnumerate)
 append(t)
 appendenum(t)
	 if varname and varname ~= "" then
		if t.name ~= "" then
			Variable(t.name.." "..varname)
		else
			local ns = getcurrnamespace()
			warning("Variable "..ns..varname.." of type <anonymous enum> is declared as read-only")
			Variable("tolua_readonly int "..varname)
		end
	end
	 local parent = classContainer.curr
	 if parent then
		t.access = parent.curr_member_access
		t.global_access = t:check_public_access()
	 end
return t
end

-- Constructor
-- Expects a string representing the enumerate body
function Enumerate (n,b,varname)
	b = string.gsub(b, ",[%s\n]*}", "\n}") -- eliminate last ','
 local t = split(strsub(b,2,-2),',') -- eliminate braces
 local i = 1
 local e = {n=0}
 while t[i] do
  local tt = split(t[i],'=')  -- discard initial value
  e.n = e.n + 1
  e[e.n] = tt[1]
  i = i+1
 end
 -- set lua names
 i  = 1
 e.lnames = {}
 local ns = getcurrnamespace()
 while e[i] do
  local t = split(e[i],'@')
  e[i] = t[1]
		if not t[2] then
		 t[2] = applyrenaming(t[1])
		end
  e.lnames[i] = t[2] or t[1]
  _global_enums[ ns..e[i] ] = (ns..e[i])
  i = i+1
 end
	e.name = n
	if n ~= "" then
		Typedef("int "..n)
	end
 return _Enumerate(e, varname)
end


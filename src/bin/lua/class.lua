-- tolua: class class
-- Written by Waldemar Celes
-- TeCGraf/PUC-Rio
-- Jul 1998
-- $Id: $

-- This code is free software; you can redistribute it and/or modify it.
-- The software provided hereunder is on an "as is" basis, and
-- the author has no obligation to provide maintenance, support, updates,
-- enhancements, or modifications.


-- Class class
-- Represents a class definition.
-- Stores the following fields:
--    name = class name
--    base = class base, if any (only single inheritance is supported)
--    {i}  = list of members
classClass = {
 classtype = 'class',
 name = '',
 base = '',
 type = '',
 btype = '',
 ctype = '',
}
classClass.__index = classClass
setmetatable(classClass,classContainer)


-- register class
function classClass:register (pre)

	if not self:check_public_access() then
		return
	end

 pre = pre or ''
 push(self)
	if _collect[self.type] then
		output(pre,'#ifdef __cplusplus\n')
  output(pre..'tolua_cclass(tolua_S,"'..self.lname..'","'..self.type..'","'..self.btype..'",'.._collect[self.type]..');')
		output(pre,'#else\n')
  output(pre..'tolua_cclass(tolua_S,"'..self.lname..'","'..self.type..'","'..self.btype..'",NULL);')
		output(pre,'#endif\n')
	else
  output(pre..'tolua_cclass(tolua_S,"'..self.lname..'","'..self.type..'","'..self.btype..'",NULL);')
	end
	if self.extra_bases then
		for k,base in ipairs(self.extra_bases) do
			-- not now
   --output(pre..' tolua_addbase(tolua_S, "'..self.type..'", "'..base..'");')
		end
	end
 output(pre..'tolua_beginmodule(tolua_S,"'..self.lname..'");')
 local i=1
 while self[i] do
  self[i]:register(pre..' ')
  i = i+1
 end
 output(pre..'tolua_endmodule(tolua_S);')
	pop()
end

-- return collection requirement
function classClass:requirecollection (t)
	if self.flags.protected_destructor or (not self:check_public_access()) then
		return false
	end
 push(self)
	local r = false
 local i=1
 while self[i] do
  r = self[i]:requirecollection(t) or r
  i = i+1
 end
	pop()
	-- only class that exports destructor can be appropriately collected
	-- classes that export constructors need to have a collector (overrided by -D flag on command line)
	if self._delete or ((not flags['D']) and self._new) then
		--t[self.type] = "tolua_collect_" .. gsub(self.type,"::","_")
		t[self.type] = "tolua_collect_" .. clean_template(self.type)
		r = true
	end
 return r
end

-- output tags
function classClass:decltype ()
 push(self)
	self.type = regtype(self.original_name or self.name)
	self.btype = typevar(self.base)
	self.ctype = 'const '..self.type
	if self.extra_bases then
		for i=1,table.getn(self.extra_bases) do
			self.extra_bases[i] = typevar(self.extra_bases[i])
		end
	end
 local i=1
 while self[i] do
  self[i]:decltype()
  i = i+1
 end
	pop()
end


-- Print method
function classClass:print (ident,close)
 print(ident.."Class{")
 print(ident.." name = '"..self.name.."',")
 print(ident.." base = '"..self.base.."';")
 print(ident.." lname = '"..self.lname.."',")
 print(ident.." type = '"..self.type.."',")
 print(ident.." btype = '"..self.btype.."',")
 print(ident.." ctype = '"..self.ctype.."',")
 local i=1
 while self[i] do
  self[i]:print(ident.." ",",")
  i = i+1
 end
 print(ident.."}"..close)
end

function classClass:set_protected_destructor(p)
	self.flags.protected_destructor = self.flags.protected_destructor or p
end

-- Internal constructor
function _Class (t)
 setmetatable(t,classClass)
 t:buildnames()
 append(t)
 return t
end

-- Constructor
-- Expects the name, the base (array) and the body of the class.
function Class (n,p,b)

	if table.getn(p) > 1 then
		b = string.sub(b, 1, -2)
		for i=2,table.getn(p),1 do
			b = b.."\n tolua_inherits "..p[i].." __"..p[i].."__;\n"
		end
		b = b.."\n}"
	end

	-- check for template
	b = string.gsub(b, "^{%s*TEMPLATE_BIND", "{\nTOLUA_TEMPLATE_BIND")
	local t,_,T,I = string.find(b, '^{%s*TOLUA_TEMPLATE_BIND%s*%(+%s*\"?([^\",]*)\"?%s*,%s*([^%)]*)%s*%)+')
	if t then

		-- remove quotes
		I = string.gsub(I, "\"", "")
		T = string.gsub(T, "\"", "")
		-- get type list
		local types = split_c_tokens(I, ",")
		-- remove TEMPLATE_BIND line
		local bs = string.gsub(b, "^{%s*TOLUA_TEMPLATE_BIND[^\n]*\n", "{\n")
			
		local Tl = split(T, " ")
		local tc = TemplateClass(n, p, bs, Tl)

		
		tc:throw(types, true)
		--for i=1,types.n do
		--	tc:throw(split_c_tokens(types[i], " "), true)
		--end
		return
	end
	
	local mbase

	if p then
		mbase = table.remove(p, 1)
		if not p[1] then p = nil end
	end

	mbase = mbase and resolve_template_types(mbase)

	local c
	local oname = string.gsub(n, "@.*$", "")
	oname = getnamespace(classContainer.curr)..oname

	if _global_classes[oname] then
		c = _global_classes[oname]
		if mbase and ((not c.base) or c.base == "") then
			c.base = mbase
		end
	else
		c = _Class(_Container{name=n, base=mbase, extra_bases=p})

		local ft = getnamespace(c.parent)..c.original_name
		append_global_type(ft, c)
	end

	push(c)
	c:parse(strsub(b,2,strlen(b)-1)) -- eliminate braces
	pop()
end


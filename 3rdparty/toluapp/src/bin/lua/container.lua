-- tolua: container abstract class
-- Written by Waldemar Celes
-- TeCGraf/PUC-Rio
-- Jul 1998
-- $Id: $

-- This code is free software; you can redistribute it and/or modify it.
-- The software provided hereunder is on an "as is" basis, and
-- the author has no obligation to provide maintenance, support, updates,
-- enhancements, or modifications.

-- table to store namespaced typedefs/enums in global scope
global_typedefs = {}
global_enums = {}

-- Container class
-- Represents a container of features to be bound
-- to lua.
classContainer =
{
 curr = nil,
}
classContainer.__index = classContainer
setmetatable(classContainer,classFeature)

-- output tags
function classContainer:decltype ()
 push(self)
 local i=1
 while self[i] do
  self[i]:decltype()
  i = i+1
 end
 pop()
end


-- write support code
function classContainer:supcode ()

	if not self:check_public_access() then
		return
	end

 push(self)
 local i=1
 while self[i] do
  if self[i]:check_public_access() then
  	self[i]:supcode()
  end
  i = i+1
 end
 pop()
end

function classContainer:hasvar ()
 local i=1
 while self[i] do
  if self[i]:isvariable() then
		 return 1
		end
  i = i+1
 end
	return 0
end

-- Internal container constructor
function _Container (self)
 setmetatable(self,classContainer)
 self.n = 0
 self.typedefs = {tolua_n=0}
 self.usertypes = {}
 self.enums = {tolua_n=0}
 self.lnames = {}
 return self
end

-- push container
function push (t)
	t.prox = classContainer.curr
 classContainer.curr = t
end

-- pop container
function pop ()
--print("name",classContainer.curr.name)
--foreach(classContainer.curr.usertypes,print)
--print("______________")
 classContainer.curr = classContainer.curr.prox
end

-- get current namespace
function getcurrnamespace ()
	return getnamespace(classContainer.curr)
end

-- append to current container
function append (t)
 return classContainer.curr:append(t)
end

-- append typedef to current container
function appendtypedef (t)
 return classContainer.curr:appendtypedef(t)
end

-- append usertype to current container
function appendusertype (t)
 return classContainer.curr:appendusertype(t)
end

-- append enum to current container
function appendenum (t)
 return classContainer.curr:appendenum(t)
end

-- substitute typedef
function applytypedef (mod,type)
 return classContainer.curr:applytypedef(mod,type)
end

-- check if is type
function findtype (type)
 local t = classContainer.curr:findtype(type)
	return t
end

-- check if is typedef
function istypedef (type)
 return classContainer.curr:istypedef(type)
end

-- get fulltype (with namespace)
function fulltype (t)
 local curr =  classContainer.curr
	while curr do
	 if curr then
		 if curr.typedefs and curr.typedefs[t] then
		  return curr.typedefs[t]
		 elseif curr.usertypes and curr.usertypes[t] then
		  return curr.usertypes[t]
			end
		end
	 curr = curr.prox
	end
	return t
end

-- checks if it requires collection
function classContainer:requirecollection (t)
 push(self)
 local i=1
	local r = false
 while self[i] do
  r = self[i]:requirecollection(t) or r
  i = i+1
 end
	pop()
	return r
end


-- get namesapce
function getnamespace (curr)
	local namespace = ''
	while curr do
	 if curr and
		   ( curr.classtype == 'class' or curr.classtype == 'namespace')
		then
		 namespace = (curr.original_name or curr.name) .. '::' .. namespace
		 --namespace = curr.name .. '::' .. namespace
		end
	 curr = curr.prox
	end
	return namespace
end

-- get namespace (only namespace)
function getonlynamespace ()
 local curr = classContainer.curr
	local namespace = ''
	while curr do
		if curr.classtype == 'class' then
		 return namespace
		elseif curr.classtype == 'namespace' then
		 namespace = curr.name .. '::' .. namespace
		end
	 curr = curr.prox
	end
	return namespace
end

-- check if is enum
function isenum (type)
 return classContainer.curr:isenum(type)
end

-- append feature to container
function classContainer:append (t)
 self.n = self.n + 1
 self[self.n] = t
 t.parent = self
end

-- append typedef
function classContainer:appendtypedef (t)
 local namespace = getnamespace(classContainer.curr)
 self.typedefs.tolua_n = self.typedefs.tolua_n + 1
 self.typedefs[self.typedefs.tolua_n] = t
	self.typedefs[t.utype] = namespace .. t.utype
	global_typedefs[namespace..t.utype] = t
	t.ftype = findtype(t.type) or t.type
	--print("appending typedef "..t.utype.." as "..namespace..t.utype.." with ftype "..t.ftype)
	append_global_type(namespace..t.utype)
	if t.ftype and isenum(t.ftype) then

		global_enums[namespace..t.utype] = true
	end
end

-- append usertype: return full type
function classContainer:appendusertype (t)
	local container
	if t == (self.original_name or self.name) then
		container = self.prox
	else
		container = self
	end
	local ft = getnamespace(container) .. t
	container.usertypes[t] = ft
	_usertype[ft] = ft
	return ft
end

-- append enum
function classContainer:appendenum (t)
 local namespace = getnamespace(classContainer.curr)
 self.enums.tolua_n = self.enums.tolua_n + 1
 self.enums[self.enums.tolua_n] = t
	global_enums[namespace..t.name] = t
end

-- determine lua function name overload
function classContainer:overload (lname)
 if not self.lnames[lname] then
  self.lnames[lname] = 0
 else
  self.lnames[lname] = self.lnames[lname] + 1
 end
 return format("%02d",self.lnames[lname])
end

-- applies typedef: returns the 'the facto' modifier and type
function classContainer:applytypedef (mod,type)
	if global_typedefs[type] then
		--print("found typedef "..global_typedefs[type].type)
		local mod1, type1 = global_typedefs[type].mod, global_typedefs[type].ftype
		local mod2, type2 = applytypedef(mod.." "..mod1, type1)
		--return mod2 .. ' ' .. mod1, type2
		return mod2, type2
	end
	do return mod,type end
end

-- check if it is a typedef
function classContainer:istypedef (type)
 local env = self
 while env do
  if env.typedefs then
   local i=1
   while env.typedefs[i] do
    if env.typedefs[i].utype == type then
         return type
        end
        i = i+1
   end
  end
  env = env.parent
 end
 return nil
end

function find_enum_var(var)

	if tonumber(var) then return var end

	local c = classContainer.curr
	while c do
		local ns = getnamespace(c)
		for k,v in pairs(_global_enums) do
			if match_type(var, v, ns) then
				return v
			end
		end
		if c.base and c.base ~= '' then
			c = _global_classes[c:findtype(c.base)]
		else
			c = nil
		end
	end

	return var
end

-- check if is a registered type: return full type or nil
function classContainer:findtype (t)

	t = string.gsub(t, "=.*", "")
	if _basic[t] then
	 return t
	end

	local _,_,em = string.find(t, "([&%*])%s*$")
	t = string.gsub(t, "%s*([&%*])%s*$", "")
	p = self
	while p and type(p)=='table' do
		local st = getnamespace(p)

		for i=_global_types.n,1,-1 do -- in reverse order

			if match_type(t, _global_types[i], st) then
				return _global_types[i]..(em or "")
			end
		end
		if p.base and p.base ~= '' and p.base ~= t then
			--print("type is "..t..", p is "..p.base.." self.type is "..self.type.." self.name is "..self.name)
			p = _global_classes[p:findtype(p.base)]
		else
			p = nil
		end
	end

	return nil
end

function append_global_type(t, class)
	_global_types.n = _global_types.n +1
	_global_types[_global_types.n] = t
	_global_types_hash[t] = 1
	if class then append_class_type(t, class) end
end

function append_class_type(t,class)
	if _global_classes[t] then
		class.flags = _global_classes[t].flags
		class.lnames = _global_classes[t].lnames
		if _global_classes[t].base and (_global_classes[t].base ~= '') then
			class.base = _global_classes[t].base or class.base
		end
	end
	_global_classes[t] = class
	class.flags = class.flags or {}
end

function match_type(childtype, regtype, st)
--print("findtype "..childtype..", "..regtype..", "..st)
	local b,e = string.find(regtype, childtype, -string.len(childtype), true)
	if b then

		if e == string.len(regtype) and
				(b == 1 or (string.sub(regtype, b-1, b-1) == ':' and
				string.sub(regtype, 1, b-1) == string.sub(st, 1, b-1))) then
			return true
		end
	end

	return false
end

function findtype_on_childs(self, t)

	local tchild
	if self.classtype == 'class' or self.classtype == 'namespace' then
		for k,v in ipairs(self) do
			if v.classtype == 'class' or v.classtype == 'namespace' then
				if v.typedefs and v.typedefs[t] then
				 return v.typedefs[t]
				elseif v.usertypes and v.usertypes[t] then
				 return v.usertypes[t]
				end
				tchild = findtype_on_childs(v, t)
				if tchild then return tchild end
			end
		end
	end
	return nil

end

function classContainer:isenum (type)
 if global_enums[type] then
	return type
 else
 	return false
 end

 local basetype = gsub(type,"^.*::","")
 local env = self
 while env do
  if env.enums then
   local i=1
   while env.enums[i] do
    if env.enums[i].name == basetype then
         return true
        end
        i = i+1
   end
  end
  env = env.parent
 end
 return false
end

methodisvirtual = false -- a global

-- parse chunk
function classContainer:doparse (s)
--print ("parse "..s)

 -- try the parser hook
 do
 	local sub = parser_hook(s)
 	if sub then
 		return sub
 	end
 end

 -- try the null statement
 do
 	local b,e,code = string.find(s, "^%s*;")
 	if b then
 		return strsub(s,e+1)
 	end
 end

 -- try empty verbatim line
 do
 	local b,e,code = string.find(s, "^%s*$\n")
 	if b then
 		return strsub(s,e+1)
 	end
 end

 -- try Lua code
 do
  local b,e,code = strfind(s,"^%s*(%b\1\2)")
  if b then
   Code(strsub(code,2,-2))
   return strsub(s,e+1)
  end
 end

 -- try C code
 do
  local b,e,code = strfind(s,"^%s*(%b\3\4)")
  if b then
	code = '{'..strsub(code,2,-2)..'\n}\n'
	Verbatim(code,'r')        -- verbatim code for 'r'egister fragment
	return strsub(s,e+1)
  end
 end

 -- try C code for preamble section
 do
 	local b,e,code = string.find(s, "^%s*(%b\5\6)")
 	if b then
 		code = string.sub(code, 2, -2).."\n"
		Verbatim(code, '')
		return string.sub(s, e+1)
 	end
 end

 -- try default_property directive
 do
 	local b,e,ptype = strfind(s, "^%s*TOLUA_PROPERTY_TYPE%s*%(+%s*([^%)%s]*)%s*%)+%s*;?")
 	if b then
 		if not ptype or ptype == "" then
 			ptype = "default"
 		end
 		self:set_property_type(ptype)
	 	return strsub(s, e+1)
 	end
 end

 -- try protected_destructor directive
 do
 	local b,e = string.find(s, "^%s*TOLUA_PROTECTED_DESTRUCTOR%s*;?")
	if b then
		if self.set_protected_destructor then
	 		self:set_protected_destructor(true)
	 	end
 		return strsub(s, e+1)
 	end
 end

 -- try 'extern' keyword
 do
 	local b,e = string.find(s, "^%s*extern%s+")
 	if b then
		-- do nothing
 		return strsub(s, e+1)
 	end
 end

 -- try 'virtual' keyworkd
 do
 	local b,e = string.find(s, "^%s*virtual%s+")
 	if b then
 		methodisvirtual = true
 		return strsub(s, e+1)
 	end
 end

 -- try labels (public, private, etc)
 do
 	local b,e = string.find(s, "^%s*%w*%s*:[^:]")
 	if b then
 		return strsub(s, e) -- preserve the [^:]
 	end
 end

 -- try module
 do
  local b,e,name,body = strfind(s,"^%s*module%s%s*([_%w][_%w]*)%s*(%b{})%s*")
  if b then
   _curr_code = strsub(s,b,e)
   Module(name,body)
   return strsub(s,e+1)
  end
 end

 -- try namesapce
 do
  local b,e,name,body = strfind(s,"^%s*namespace%s%s*([_%w][_%w]*)%s*(%b{})%s*;?")
  if b then
   _curr_code = strsub(s,b,e)
   Namespace(name,body)
   return strsub(s,e+1)
  end
 end

 -- try define
 do
  local b,e,name = strfind(s,"^%s*#define%s%s*([^%s]*)[^\n]*\n%s*")
  if b then
   _curr_code = strsub(s,b,e)
   Define(name)
   return strsub(s,e+1)
  end
 end

 -- try enumerates

 do
  local b,e,name,body,varname = strfind(s,"^%s*enum%s+(%S*)%s*(%b{})%s*([^%s;]*)%s*;?%s*")
  if b then
   --error("#Sorry, declaration of enums and variables on the same statement is not supported.\nDeclare your variable separately (example: '"..name.." "..varname..";')")
   _curr_code = strsub(s,b,e)
   Enumerate(name,body,varname)
   return strsub(s,e+1)
  end
 end

-- do
--  local b,e,name,body = strfind(s,"^%s*enum%s+(%S*)%s*(%b{})%s*;?%s*")
--  if b then
--   _curr_code = strsub(s,b,e)
--   Enumerate(name,body)
--  return strsub(s,e+1)
--  end
-- end

 do
  local b,e,body,name = strfind(s,"^%s*typedef%s+enum[^{]*(%b{})%s*([%w_][^%s]*)%s*;%s*")
  if b then
   _curr_code = strsub(s,b,e)
   Enumerate(name,body)
   return strsub(s,e+1)
  end
 end

 -- try operator
 do
  local b,e,decl,kind,arg,const = strfind(s,"^%s*([_%w][_%w%s%*&:<>,]-%s+operator)%s*([^%s][^%s]*)%s*(%b())%s*(c?o?n?s?t?)%s*;%s*")
  if not b then
		 -- try inline
   b,e,decl,kind,arg,const = strfind(s,"^%s*([_%w][_%w%s%*&:<>,]-%s+operator)%s*([^%s][^%s]*)%s*(%b())%s*(c?o?n?s?t?)[%s\n]*%b{}%s*;?%s*")
  end
  if not b then
  	-- try cast operator
  	b,e,decl,kind,arg,const = strfind(s, "^%s*(operator)%s+([%w_:%d<>%*%&%s]+)%s*(%b())%s*(c?o?n?s?t?)");
  	if b then
  		local _,ie = string.find(s, "^%s*%b{}", e+1)
  		if ie then
  			e = ie
  		end
  	end
  end
  if b then
   _curr_code = strsub(s,b,e)
   Operator(decl,kind,arg,const)
   return strsub(s,e+1)
  end
 end

 -- try function
 do
  --local b,e,decl,arg,const = strfind(s,"^%s*([~_%w][_@%w%s%*&:<>]*[_%w])%s*(%b())%s*(c?o?n?s?t?)%s*=?%s*0?%s*;%s*")
  local b,e,decl,arg,const,virt = strfind(s,"^%s*([^%(\n]+)%s*(%b())%s*(c?o?n?s?t?)%s*(=?%s*0?)%s*;%s*")
  if not b then
  	-- try function with template
  	b,e,decl,arg,const = strfind(s,"^%s*([~_%w][_@%w%s%*&:<>]*[_%w]%b<>)%s*(%b())%s*(c?o?n?s?t?)%s*=?%s*0?%s*;%s*")
  end
  if not b then
   -- try a single letter function name
   b,e,decl,arg,const = strfind(s,"^%s*([_%w])%s*(%b())%s*(c?o?n?s?t?)%s*;%s*")
  end
  if not b then
   -- try function pointer
   b,e,decl,arg,const = strfind(s,"^%s*([^%(;\n]+%b())%s*(%b())%s*;%s*")
   if b then
    decl = string.gsub(decl, "%(%s*%*([^%)]*)%s*%)", " %1 ")
   end
  end
  if b then
  	if virt and string.find(virt, "[=0]") then
  		if self.flags then
  			self.flags.pure_virtual = true
  		end
  	end
   _curr_code = strsub(s,b,e)
   Function(decl,arg,const)
   return strsub(s,e+1)
  end
 end

 -- try inline function
 do
  local b,e,decl,arg,const = strfind(s,"^%s*([^%(\n]+)%s*(%b())%s*(c?o?n?s?t?)[^;{]*%b{}%s*;?%s*")
  --local b,e,decl,arg,const = strfind(s,"^%s*([~_%w][_@%w%s%*&:<>]*[_%w>])%s*(%b())%s*(c?o?n?s?t?)[^;]*%b{}%s*;?%s*")
  if not b then
   -- try a single letter function name
   b,e,decl,arg,const = strfind(s,"^%s*([_%w])%s*(%b())%s*(c?o?n?s?t?).-%b{}%s*;?%s*")
  end
  if b then
   _curr_code = strsub(s,b,e)
   Function(decl,arg,const)
   return strsub(s,e+1)
  end
 end

 -- try class
 do
	 local b,e,name,base,body
		base = '' body = ''
		b,e,name = strfind(s,"^%s*class%s*([_%w][_%w@]*)%s*;")  -- dummy class
		local dummy = false
		if not b then
			b,e,name = strfind(s,"^%s*struct%s*([_%w][_%w@]*)%s*;")    -- dummy struct
			if not b then
				b,e,name,base,body = strfind(s,"^%s*class%s*([_%w][_%w@]*)%s*([^{]-)%s*(%b{})%s*")
				if not b then
					b,e,name,base,body = strfind(s,"^%s*struct%s+([_%w][_%w@]*)%s*([^{]-)%s*(%b{})%s*")
					if not b then
						b,e,name,base,body = strfind(s,"^%s*union%s*([_%w][_%w@]*)%s*([^{]-)%s*(%b{})%s*")
						if not b then
							base = ''
							b,e,body,name = strfind(s,"^%s*typedef%s%s*struct%s%s*[_%w]*%s*(%b{})%s*([_%w][_%w@]*)%s*;")
						end
					end
				end
			else dummy = 1 end
		else dummy = 1 end
		if b then
			if base ~= '' then
				base = string.gsub(base, "^%s*:%s*", "")
				base = string.gsub(base, "%s*public%s*", "")
				base = split(base, ",")
				--local b,e
				--b,e,base = strfind(base,".-([_%w][_%w<>,:]*)$")
			else
				base = {}
			end
			_curr_code = strsub(s,b,e)
			Class(name,base,body)
			if not dummy then
				varb,vare,varname = string.find(s, "^%s*([_%w]+)%s*;", e+1)
				if varb then
					Variable(name.." "..varname)
					e = vare
				end
			end
			return strsub(s,e+1)
		end
	end

 -- try typedef
 do
  local b,e,types = strfind(s,"^%s*typedef%s%s*(.-)%s*;%s*")
  if b then
   _curr_code = strsub(s,b,e)
   Typedef(types)
   return strsub(s,e+1)
  end
 end

 -- try variable
 do
  local b,e,decl = strfind(s,"^%s*([_%w][_@%s%w%d%*&:<>,]*[_%w%d])%s*;%s*")
  if b then
   _curr_code = strsub(s,b,e)

	local list = split_c_tokens(decl, ",")
	Variable(list[1])
	if list.n > 1 then
		local _,_,type = strfind(list[1], "(.-)%s+([^%s]*)$");

		local i =2;
		while list[i] do
			Variable(type.." "..list[i])
			i=i+1
		end
	end
   --Variable(decl)
   return strsub(s,e+1)
  end
 end

	-- try string
 do
  local b,e,decl = strfind(s,"^%s*([_%w]?[_%s%w%d]-char%s+[_@%w%d]*%s*%[%s*%S+%s*%])%s*;%s*")
  if b then
   _curr_code = strsub(s,b,e)
   Variable(decl)
   return strsub(s,e+1)
  end
 end

 -- try array
 do
  local b,e,decl = strfind(s,"^%s*([_%w][][_@%s%w%d%*&:<>]*[]_%w%d])%s*;%s*")
  if b then
   _curr_code = strsub(s,b,e)
   Array(decl)
   return strsub(s,e+1)
  end
 end

 -- no matching
 if gsub(s,"%s%s*","") ~= "" then
  _curr_code = s
  error("#parse error")
 else
  return ""
 end

end

function classContainer:parse (s)

	--self.curr_member_access = nil

 while s ~= '' do
  s = self:doparse(s)
  methodisvirtual = false
 end
end


-- property types

function get_property_type()

	return classContainer.curr:get_property_type()
end

function classContainer:set_property_type(ptype)
	ptype = string.gsub(ptype, "^%s*", "")
	ptype = string.gsub(ptype, "%s*$", "")

	self.property_type = ptype
end

function classContainer:get_property_type()
	return self.property_type or (self.parent and self.parent:get_property_type()) or "default"
end

-- tolua: array class
-- Written by Waldemar Celes
-- TeCGraf/PUC-Rio
-- Jul 1999
-- $Id: array.lua,v 1.1 2000/11/06 22:03:57 celes Exp $

-- This code is free software; you can redistribute it and/or modify it.
-- The software provided hereunder is on an "as is" basis, and
-- the author has no obligation to provide maintenance, support, updates,
-- enhancements, or modifications.


-- Array class
-- Represents a extern array variable or a public member of a class.
-- Stores all fields present in a declaration.
classArray = {
}
classArray.__index = classArray
setmetatable(classArray,classDeclaration)

-- Print method
function classArray:print (ident,close)
 print(ident.."Array{")
 print(ident.." mod  = '"..self.mod.."',")
 print(ident.." type = '"..self.type.."',")
 print(ident.." ptr  = '"..self.ptr.."',")
 print(ident.." name = '"..self.name.."',")
 print(ident.." def  = '"..self.def.."',")
 print(ident.." dim  = '"..self.dim.."',")
 print(ident.." ret  = '"..self.ret.."',")
 print(ident.."}"..close)
end

-- check if it is a variable
function classArray:isvariable ()
 return true
end


-- get variable value
function classArray:getvalue (class,static)
 if class and static then
  return class..'::'..self.name..'[tolua_index]'
 elseif class then
  return 'self->'..self.name..'[tolua_index]'
 else
  return self.name..'[tolua_index]'
 end
end

-- Write binding functions
function classArray:supcode ()
 local class = self:inclass()

 -- get function ------------------------------------------------
 if class then
  output("/* get function:",self.name," of class ",class," */")
 else
  output("/* get function:",self.name," */")
 end
 self.cgetname = self:cfuncname("tolua_get")
 output("#ifndef TOLUA_DISABLE_"..self.cgetname)
 output("\nstatic int",self.cgetname,"(lua_State* tolua_S)")
 output("{")
 output(" int tolua_index;")

 -- declare self, if the case
 local _,_,static = strfind(self.mod,'^%s*(static)')
 if class and static==nil then
  output(' ',self.parent.type,'*','self;')
  output(' lua_pushstring(tolua_S,".self");')
  output(' lua_rawget(tolua_S,1);')
  output(' self = ')
  output('(',self.parent.type,'*) ')
  output('lua_touserdata(tolua_S,-1);')
 elseif static then
  _,_,self.mod = strfind(self.mod,'^%s*static%s%s*(.*)')
 end

 -- check index
	output('#ifndef TOLUA_RELEASE\n')
	output(' {')
	output('  tolua_Error tolua_err;')
 output('  if (!tolua_isnumber(tolua_S,2,0,&tolua_err))')
 output('   tolua_error(tolua_S,"#vinvalid type in array indexing.",&tolua_err);')
	output(' }')
	output('#endif\n')
	if flags['1'] then -- for compatibility with tolua5 ?
		output(' tolua_index = (int)tolua_tonumber(tolua_S,2,0)-1;')
	else
		output(' tolua_index = (int)tolua_tonumber(tolua_S,2,0);')
	end
	output('#ifndef TOLUA_RELEASE\n')
	if self.dim and self.dim ~= '' then
	  output(' if (tolua_index<0 || tolua_index>='..self.dim..')')
	else
	  output(' if (tolua_index<0)')
	end
 output('  tolua_error(tolua_S,"array indexing out of range.",NULL);')
	output('#endif\n')

 -- return value
 local t,ct = isbasic(self.type)
 local push_func = get_push_function(t)
 if t then
  output(' tolua_push'..t..'(tolua_S,(',ct,')'..self:getvalue(class,static)..');')
 else
		t = self.type
  if self.ptr == '&' or self.ptr == '' then
   output(' ',push_func,'(tolua_S,(void*)&'..self:getvalue(class,static)..',"',t,'");')
  else
   output(' ',push_func,'(tolua_S,(void*)'..self:getvalue(class,static)..',"',t,'");')
  end
 end
 output(' return 1;')
 output('}')
 output('#endif //#ifndef TOLUA_DISABLE\n')
 output('\n')

 -- set function ------------------------------------------------
 if not strfind(self.type,'const') then
  if class then
   output("/* set function:",self.name," of class ",class," */")
  else
   output("/* set function:",self.name," */")
  end
  self.csetname = self:cfuncname("tolua_set")
  output("#ifndef TOLUA_DISABLE_"..self.csetname)
  output("\nstatic int",self.csetname,"(lua_State* tolua_S)")
  output("{")

  -- declare index
  output(' int tolua_index;')

  -- declare self, if the case
  local _,_,static = strfind(self.mod,'^%s*(static)')
  if class and static==nil then
   output(' ',self.parent.type,'*','self;')
   output(' lua_pushstring(tolua_S,".self");')
   output(' lua_rawget(tolua_S,1);')
   output(' self = ')
   output('(',self.parent.type,'*) ')
   output('lua_touserdata(tolua_S,-1);')
  elseif static then
   _,_,self.mod = strfind(self.mod,'^%s*static%s%s*(.*)')
  end

  -- check index
	 output('#ifndef TOLUA_RELEASE\n')
	 output(' {')
	 output('  tolua_Error tolua_err;')
  output('  if (!tolua_isnumber(tolua_S,2,0,&tolua_err))')
  output('   tolua_error(tolua_S,"#vinvalid type in array indexing.",&tolua_err);')
		output(' }')
		output('#endif\n')

	if flags['1'] then -- for compatibility with tolua5 ?
		output(' tolua_index = (int)tolua_tonumber(tolua_S,2,0)-1;')
	else
		output(' tolua_index = (int)tolua_tonumber(tolua_S,2,0);')
	end

	 output('#ifndef TOLUA_RELEASE\n')
	if self.dim and self.dim ~= '' then
	  output(' if (tolua_index<0 || tolua_index>='..self.dim..')')
	else
	  output(' if (tolua_index<0)')
	end
  output('  tolua_error(tolua_S,"array indexing out of range.",NULL);')
		output('#endif\n')

  -- assign value
  local ptr = ''
  if self.ptr~='' then ptr = '*' end
  output(' ')
  if class and static then
   output(class..'::'..self.name..'[tolua_index]')
  elseif class then
   output('self->'..self.name..'[tolua_index]')
  else
   output(self.name..'[tolua_index]')
  end
  local t = isbasic(self.type)
  output(' = ')
  if not t and ptr=='' then output('*') end
  output('((',self.mod,self.type)
  if not t then
   output('*')
  end
  output(') ')
  local def = 0
  if self.def ~= '' then def = self.def end
  if t then
   output('tolua_to'..t,'(tolua_S,3,',def,'));')
  else
   local to_func = get_to_function(self.type)
   output(to_func,'(tolua_S,3,',def,'));')
  end
  output(' return 0;')
  output('}')
  output('#endif //#ifndef TOLUA_DISABLE\n')
  output('\n')
 end

end

function classArray:register (pre)
	if not self:check_public_access() then
		return
	end

 pre = pre or ''
 if self.csetname then
  output(pre..'tolua_array(tolua_S,"'..self.lname..'",'..self.cgetname..','..self.csetname..');')
 else
  output(pre..'tolua_array(tolua_S,"'..self.lname..'",'..self.cgetname..',NULL);')
 end
end

-- Internal constructor
function _Array (t)
 setmetatable(t,classArray)
 append(t)
 return t
end

-- Constructor
-- Expects a string representing the variable declaration.
function Array (s)
 return _Array (Declaration(s,'var'))
end



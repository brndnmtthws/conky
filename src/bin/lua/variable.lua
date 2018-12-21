-- tolua: variable class
-- Written by Waldemar Celes
-- TeCGraf/PUC-Rio
-- Jul 1998
-- $Id: $

-- This code is free software; you can redistribute it and/or modify it.
-- The software provided hereunder is on an "as is" basis, and
-- the author has no obligation to provide maintenance, support, updates,
-- enhancements, or modifications.


-- Variable class
-- Represents a extern variable or a public member of a class.
-- Stores all fields present in a declaration.
classVariable = {
 _get = {},   -- mapped get functions
 _set = {},   -- mapped set functions
}
classVariable.__index = classVariable
setmetatable(classVariable,classDeclaration)

-- Print method
function classVariable:print (ident,close)
 print(ident.."Variable{")
 print(ident.." mod  = '"..self.mod.."',")
 print(ident.." type = '"..self.type.."',")
 print(ident.." ptr  = '"..self.ptr.."',")
 print(ident.." name = '"..self.name.."',")
 if self.dim then print(ident.." dim = '"..self.dim.."',") end
 print(ident.." def  = '"..self.def.."',")
 print(ident.." ret  = '"..self.ret.."',")
 print(ident.."}"..close)
end

-- Generates C function name
function classVariable:cfuncname (prefix)
 local parent = ""
 local unsigned = ""
 local ptr = ""

 local p = self:inmodule() or self:innamespace() or self:inclass()

 if p then
 	if self.parent.classtype == 'class' then
		parent = "_" .. self.parent.type
	else
	  parent = "_" .. p
	end
 end

 if strfind(self.mod,"(unsigned)") then
  unsigned = "_unsigned"
 end

 if self.ptr == "*" then ptr = "_ptr"
 elseif self.ptr == "&" then ptr = "_ref"
 end

 local name =  prefix .. parent .. unsigned .. "_" .. gsub(self.lname or self.name,".*::","") .. ptr

	name = clean_template(name)
 return name

end

-- check if it is a variable
function classVariable:isvariable ()
 return true
end

-- get variable value
function classVariable:getvalue (class,static, prop_get)

	local name
	if prop_get then

		name = prop_get.."()"
	else
		name = self.name
	end

	if class and static then
	 return self.parent.type..'::'..name
	elseif class then
	 return 'self->'..name
	else
	 return name
	end
end

-- get variable pointer value
function classVariable:getpointervalue (class,static)
 if class and static then
  return class..'::p'
 elseif class then
  return 'self->p'
 else
  return 'p'
 end
end

-- Write binding functions
function classVariable:supcode ()

 local class = self:inclass()

	local prop_get,prop_set
	if string.find(self.mod, 'tolua_property') then

		local _,_,type = string.find(self.mod, "tolua_property__([^%s]*)")
		type = type or "default"
		prop_get,prop_set = get_property_methods(type, self.name)
		self.mod = string.gsub(self.mod, "tolua_property[^%s]*", "")
	end

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

 -- declare self, if the case
 local _,_,static = strfind(self.mod,'^%s*(static)')
 if class and static==nil then
  output(' ',self.parent.type,'*','self = ')
  output('(',self.parent.type,'*) ')
  local to_func = get_to_function(self.parent.type)
  output(to_func,'(tolua_S,1,0);')
 elseif static then
  _,_,self.mod = strfind(self.mod,'^%s*static%s%s*(.*)')
 end


 -- check self value
 if class and static==nil then
	 output('#ifndef TOLUA_RELEASE\n')
  output('  if (!self) tolua_error(tolua_S,"'..output_error_hook("invalid \'self\' in accessing variable \'%s\'", self.name)..'",NULL);');
		output('#endif\n')
 end

 -- return value
 if string.find(self.mod, 'tolua_inherits') then
	local push_func = get_push_function(self.type)
 	output('#ifdef __cplusplus\n')
	output('  ',push_func,'(tolua_S,(void*)static_cast<'..self.type..'*>(self), "',self.type,'");')
	output('#else\n')
	output('  ',push_func,'(tolua_S,(void*)(('..self.type..'*)self), "',self.type,'");')
	output('#endif\n')
 else
	local t,ct = isbasic(self.type)
	if t then
		output('  tolua_push'..t..'(tolua_S,(',ct,')'..self:getvalue(class,static,prop_get)..');')
	else
		local push_func = get_push_function(self.type)
		t = self.type
		if self.ptr == '&' or self.ptr == '' then
			output('  ',push_func,'(tolua_S,(void*)&'..self:getvalue(class,static,prop_get)..',"',t,'");')
		else
			output('  ',push_func,'(tolua_S,(void*)'..self:getvalue(class,static,prop_get)..',"',t,'");')
		end
	end
 end
 output(' return 1;')
 output('}')
 output('#endif //#ifndef TOLUA_DISABLE\n')
 output('\n')

 -- set function ------------------------------------------------
 if not (strfind(self.type,'const%s+') or string.find(self.mod, 'tolua_readonly') or string.find(self.mod, 'tolua_inherits'))  then
  if class then
   output("/* set function:",self.name," of class ",class," */")
  else
   output("/* set function:",self.name," */")
  end
  self.csetname = self:cfuncname("tolua_set")
  output("#ifndef TOLUA_DISABLE_"..self.csetname)
  output("\nstatic int",self.csetname,"(lua_State* tolua_S)")
  output("{")

  -- declare self, if the case
  if class and static==nil then
   output(' ',self.parent.type,'*','self = ')
   output('(',self.parent.type,'*) ')
   local to_func = get_to_function(self.parent.type)
   output(to_func,'(tolua_S,1,0);')
   -- check self value
		end
  -- check types
		output('#ifndef TOLUA_RELEASE\n')
		output('  tolua_Error tolua_err;')
  if class and static==nil then
   output('  if (!self) tolua_error(tolua_S,"'..output_error_hook("invalid \'self\' in accessing variable \'%s\'", self.name)..'",NULL);');
  elseif static then
   _,_,self.mod = strfind(self.mod,'^%s*static%s%s*(.*)')
  end

  -- check variable type
  output('  if ('..self:outchecktype(2)..')')
  output('   tolua_error(tolua_S,"#vinvalid type in variable assignment.",&tolua_err);')
		output('#endif\n')

  -- assign value
		local def = 0
		if self.def ~= '' then def = self.def end
		if self.type == 'char*' and self.dim ~= '' then -- is string
			output(' strncpy((char*)')
			if class and static then
				output(self.parent.type..'::'..self.name)
			elseif class then
				output('self->'..self.name)
			else
				output(self.name)
			end
			output(',(const char*)tolua_tostring(tolua_S,2,',def,'),',self.dim,'-1);')
		else
			local ptr = ''
			if self.ptr~='' then ptr = '*' end
			output(' ')
			local name = prop_set or self.name
			if class and static then
				output(self.parent.type..'::'..name)
			elseif class then
				output('self->'..name)
			else
				output(name)
			end
			local t = isbasic(self.type)
			if prop_set then
				output('(')
			else
				output(' = ')
			end
			if not t and ptr=='' then output('*') end
			output('((',self.mod,self.type)
			if not t then
				output('*')
			end
			output(') ')
			if t then
				if isenum(self.type) then
					output('(int) ')
				end
				output('tolua_to'..t,'(tolua_S,2,',def,'))')
			else
				local to_func = get_to_function(self.type)
				output(to_func,'(tolua_S,2,',def,'))')
			end
			if prop_set then
				output(")")
			end
			output(";")
		end
  output(' return 0;')
  output('}')
  output('#endif //#ifndef TOLUA_DISABLE\n')
  output('\n')
 end

end

function classVariable:register (pre)

	if not self:check_public_access() then
		return
	end
 pre = pre or ''
 local parent = self:inmodule() or self:innamespace() or self:inclass()
 if not parent then
  if classVariable._warning==nil then
   warning("Mapping variable to global may degrade performance")
   classVariable._warning = 1
  end
 end
 if self.csetname then
  output(pre..'tolua_variable(tolua_S,"'..self.lname..'",'..self.cgetname..','..self.csetname..');')
 else
  output(pre..'tolua_variable(tolua_S,"'..self.lname..'",'..self.cgetname..',NULL);')
 end
end

-- Internal constructor
function _Variable (t)
 setmetatable(t,classVariable)
 append(t)
 return t
end

-- Constructor
-- Expects a string representing the variable declaration.
function Variable (s)
 return _Variable (Declaration(s,'var'))
end



-- tolua: declaration class
-- Written by Waldemar Celes
-- TeCGraf/PUC-Rio
-- Jul 1998
-- $Id: $

-- This code is free software; you can redistribute it and/or modify it.
-- The software provided hereunder is on an "as is" basis, and
-- the author has no obligation to provide maintenance, support, updates,
-- enhancements, or modifications.


-- Declaration class
-- Represents variable, function, or argument declaration.
-- Stores the following fields:
--  mod  = type modifiers
--  type = type
--  ptr  = "*" or "&", if representing a pointer or a reference
--  name = name
--  dim  = dimension, if a vector
--  def  = default value, if any (only for arguments)
--  ret  = "*" or "&", if value is to be returned (only for arguments)
classDeclaration = {
 mod = '',
 type = '',
 ptr = '',
 name = '',
 dim = '',
 ret = '',
 def = ''
}
classDeclaration.__index = classDeclaration
setmetatable(classDeclaration,classFeature)

-- Create an unique variable name
function create_varname ()
 if not _varnumber then _varnumber = 0 end
 _varnumber = _varnumber + 1
 return "tolua_var_".._varnumber
end

-- Check declaration name
-- It also identifies default values
function classDeclaration:checkname ()

 if strsub(self.name,1,1) == '[' and not findtype(self.type) then
  self.name = self.type..self.name
  local m = split(self.mod,'%s%s*')
  self.type = m[m.n]
  self.mod = concat(m,1,m.n-1)
 end

 local t = split(self.name,'=')
 if t.n==2 then
  self.name = t[1]
  self.def = find_enum_var(t[t.n])
 end

 local b,e,d = strfind(self.name,"%[(.-)%]")
 if b then
  self.name = strsub(self.name,1,b-1)
  self.dim = find_enum_var(d)
 end


 if self.type ~= '' and self.type ~= 'void' and self.name == '' then
  self.name = create_varname()
 elseif self.kind=='var' then
  if self.type=='' and self.name~='' then
   self.type = self.type..self.name
   self.name = create_varname()
  elseif findtype(self.name) then
   if self.type=='' then self.type = self.name
   else self.type = self.type..' '..self.name end
   self.name = create_varname()
  end
 end

 -- adjust type of string
 if self.type == 'char' and self.dim ~= '' then
	 self.type = 'char*'
 end

	if self.kind and self.kind == 'var' then
		self.name = string.gsub(self.name, ":.*$", "") -- ???
	end
end

-- Check declaration type
-- Substitutes typedef's.
function classDeclaration:checktype ()

 -- check if there is a pointer to basic type
 local basic = isbasic(self.type)
 if self.kind == 'func' and basic=='number' and string.find(self.ptr, "%*") then
 	self.type = '_userdata'
 	self.ptr = ""
 end
 if basic and self.ptr~='' then
  self.ret = self.ptr
  self.ptr = nil
  if isbasic(self.type) == 'number' then
  	self.return_userdata = true
  end
 end

 -- check if there is array to be returned
 if self.dim~='' and self.ret~='' then
   error('#invalid parameter: cannot return an array of values')
 end
 -- restore 'void*' and 'string*'
 if self.type == '_userdata' then self.type = 'void*'
 elseif self.type == '_cstring' then self.type = 'char*'
 elseif self.type == '_lstate' then self.type = 'lua_State*'
 end

 -- resolve types inside the templates
 if self.type then
	 self.type = resolve_template_types(self.type)
 end

--
-- -- if returning value, automatically set default value
-- if self.ret ~= '' and self.def == '' then
--  self.def = '0'
-- end
--

end

function resolve_template_types(type)

	if isbasic(type) then
		return type
	end
	local b,_,m = string.find(type, "(%b<>)")
	if b then

		m = split_c_tokens(string.sub(m, 2, -2), ",")
		for i=1, table.getn(m) do
			m[i] = string.gsub(m[i],"%s*([%*&])", "%1")
			if not isbasic(m[i]) then
				if not isenum(m[i]) then _, m[i] = applytypedef("", m[i]) end
				m[i] = findtype(m[i]) or m[i]
				m[i] = resolve_template_types(m[i])
			end
		end

		local b,i
		type,b,i = break_template(type)
--print("concat is ",concat(m, 1, m.n))
		local template_part = "<"..concat(m, 1, m.n, ",")..">"
		type = rebuild_template(type, b, template_part)
		type = string.gsub(type, ">>", "> >")
	end
	return type
end

function break_template(s)
	local b,e,timpl = string.find(s, "(%b<>)")
	if timpl then
		s = string.gsub(s, "%b<>", "")
		return s, b, timpl
	else
		return s, 0, nil
	end
end

function rebuild_template(s, b, timpl)

	if b == 0 then
		return s
	end

	return string.sub(s, 1, b-1)..timpl..string.sub(s, b, -1)
end

-- Print method
function classDeclaration:print (ident,close)
 print(ident.."Declaration{")
 print(ident.." mod  = '"..self.mod.."',")
 print(ident.." type = '"..self.type.."',")
 print(ident.." ptr  = '"..self.ptr.."',")
 print(ident.." name = '"..self.name.."',")
 print(ident.." dim  = '"..self.dim.."',")
 print(ident.." def  = '"..self.def.."',")
 print(ident.." ret  = '"..self.ret.."',")
 print(ident.."}"..close)
end

-- check if array of values are returned to Lua
function classDeclaration:requirecollection (t)
 if self.mod ~= 'const' and
	    self.dim and self.dim ~= '' and
				 not isbasic(self.type) and
				 self.ptr == '' and self:check_public_access() then
		local type = gsub(self.type,"%s*const%s+","")
		t[type] = "tolua_collect_" .. clean_template(type)
		return true
	end
	return false
end

-- declare tag
function classDeclaration:decltype ()

	self.type = typevar(self.type)
	if strfind(self.mod,'const') then
		self.type = 'const '..self.type
		self.mod = gsub(self.mod,'const%s*','')
	end
end


-- output type checking
function classDeclaration:outchecktype (narg)
 local def
 local t = isbasic(self.type)
 if self.def~='' then
  def = 1
 else
  def = 0
 end
 if self.dim ~= '' then
	--if t=='string' then
	--	return 'tolua_isstringarray(tolua_S,'..narg..','..def..',&tolua_err)'
	--else
	return '!tolua_istable(tolua_S,'..narg..',0,&tolua_err)'
 	--end
 elseif t then
	return '!tolua_is'..t..'(tolua_S,'..narg..','..def..',&tolua_err)'
 else
  local is_func = get_is_function(self.type)
  if self.ptr == '&' or self.ptr == '' then
  	return '(tolua_isvaluenil(tolua_S,'..narg..',&tolua_err) || !'..is_func..'(tolua_S,'..narg..',"'..self.type..'",'..def..',&tolua_err))'
  else
	return '!'..is_func..'(tolua_S,'..narg..',"'..self.type..'",'..def..',&tolua_err)'
  end
 end
end

function classDeclaration:builddeclaration (narg, cplusplus)
 local array = self.dim ~= '' and tonumber(self.dim)==nil
	local line = ""
 local ptr = ''
 local mod
 local type = self.type
 local nctype = gsub(self.type,'const%s+','')
 if self.dim ~= '' then
	 type = gsub(self.type,'const%s+','')  -- eliminates const modifier for arrays
 end
 if self.ptr~='' and not isbasic(type) then ptr = '*' end
 line = concatparam(line," ",self.mod,type,ptr)
 if array then
  line = concatparam(line,'*')
 end
 line = concatparam(line,self.name)
 if self.dim ~= '' then
  if tonumber(self.dim)~=nil then
   line = concatparam(line,'[',self.dim,'];')
  else
	if cplusplus then
		line = concatparam(line,' = Mtolua_new_dim(',type,ptr,', '..self.dim..');')
	else
		line = concatparam(line,' = (',type,ptr,'*)',
		'malloc((',self.dim,')*sizeof(',type,ptr,'));')
	end
  end
 else
  local t = isbasic(type)
  line = concatparam(line,' = ')
  if t == 'state' then
  	line = concatparam(line, 'tolua_S;')
  else
  	--print("t is "..tostring(t)..", ptr is "..tostring(self.ptr))
  	if t == 'number' and string.find(self.ptr, "%*") then
  		t = 'userdata'
  	end
	if not t and ptr=='' then line = concatparam(line,'*') end
	line = concatparam(line,'((',self.mod,type)
	if not t then
		line = concatparam(line,'*')
	end
	line = concatparam(line,') ')
	if isenum(nctype) then
		line = concatparam(line,'(int) ')
	end
	local def = 0
	if self.def ~= '' then
		def = self.def
		if (ptr == '' or self.ptr == '&') and not t then
			def = "(void*)&(const "..type..")"..def
		end
	end
	if t then
		line = concatparam(line,'tolua_to'..t,'(tolua_S,',narg,',',def,'));')
	else
		local to_func = get_to_function(type)
		line = concatparam(line,to_func..'(tolua_S,',narg,',',def,'));')
	end
  end
 end
	return line
end

-- Declare variable
function classDeclaration:declare (narg)
 if self.dim ~= '' and tonumber(self.dim)==nil then
	 output('#ifdef __cplusplus\n')
		output(self:builddeclaration(narg,true))
		output('#else\n')
		output(self:builddeclaration(narg,false))
	 output('#endif\n')
	else
		output(self:builddeclaration(narg,false))
	end
end

-- Get parameter value
function classDeclaration:getarray (narg)
 if self.dim ~= '' then
	 local type = gsub(self.type,'const ','')
  output('  {')
	 output('#ifndef TOLUA_RELEASE\n')
  local def; if self.def~='' then def=1 else def=0 end
		local t = isbasic(type)
		if (t) then
		   output('   if (!tolua_is'..t..'array(tolua_S,',narg,',',self.dim,',',def,',&tolua_err))')
		else
		   output('   if (!tolua_isusertypearray(tolua_S,',narg,',"',type,'",',self.dim,',',def,',&tolua_err))')
		end
  output('    goto tolua_lerror;')
  output('   else\n')
	 output('#endif\n')
  output('   {')
  output('    int i;')
  output('    for(i=0; i<'..self.dim..';i++)')
  local t = isbasic(type)
  local ptr = ''
  if self.ptr~='' then ptr = '*' end
  output('   ',self.name..'[i] = ')
  if not t and ptr=='' then output('*') end
  output('((',type)
  if not t then
   output('*')
  end
  output(') ')
  local def = 0
  if self.def ~= '' then def = self.def end
  if t then
   output('tolua_tofield'..t..'(tolua_S,',narg,',i+1,',def,'));')
  else
   output('tolua_tofieldusertype(tolua_S,',narg,',i+1,',def,'));')
  end
  output('   }')
  output('  }')
 end
end

-- Get parameter value
function classDeclaration:setarray (narg)
 if not strfind(self.type,'const%s+') and self.dim ~= '' then
	 local type = gsub(self.type,'const ','')
  output('  {')
  output('   int i;')
  output('   for(i=0; i<'..self.dim..';i++)')
  local t,ct = isbasic(type)
  if t then
   output('    tolua_pushfield'..t..'(tolua_S,',narg,',i+1,(',ct,')',self.name,'[i]);')
  else
   if self.ptr == '' then
     output('   {')
     output('#ifdef __cplusplus\n')
     output('    void* tolua_obj = Mtolua_new((',type,')(',self.name,'[i]));')
     output('    tolua_pushfieldusertype_and_takeownership(tolua_S,',narg,',i+1,tolua_obj,"',type,'");')
     output('#else\n')
     output('    void* tolua_obj = tolua_copy(tolua_S,(void*)&',self.name,'[i],sizeof(',type,'));')
     output('    tolua_pushfieldusertype(tolua_S,',narg,',i+1,tolua_obj,"',type,'");')
     output('#endif\n')
     output('   }')
   else
    output('   tolua_pushfieldusertype(tolua_S,',narg,',i+1,(void*)',self.name,'[i],"',type,'");')
   end
  end
  output('  }')
 end
end

-- Free dynamically allocated array
function classDeclaration:freearray ()
 if self.dim ~= '' and tonumber(self.dim)==nil then
	 output('#ifdef __cplusplus\n')
		output('  Mtolua_delete_dim(',self.name,');')
	 output('#else\n')
  output('  free(',self.name,');')
	 output('#endif\n')
 end
end

-- Pass parameter
function classDeclaration:passpar ()
 if self.ptr=='&' and not isbasic(self.type) then
  output('*'..self.name)
 elseif self.ret=='*' then
  output('&'..self.name)
 else
  output(self.name)
 end
end

-- Return parameter value
function classDeclaration:retvalue ()
 if self.ret ~= '' then
  local t,ct = isbasic(self.type)
  if t and t~='' then
   output('   tolua_push'..t..'(tolua_S,(',ct,')'..self.name..');')
  else
   local push_func = get_push_function(self.type)
   output('   ',push_func,'(tolua_S,(void*)'..self.name..',"',self.type,'");')
  end
  return 1
 end
 return 0
end

-- Internal constructor
function _Declaration (t)

 setmetatable(t,classDeclaration)
 t:buildnames()
 t:checkname()
 t:checktype()
 local ft = findtype(t.type) or t.type
 if not isenum(ft) then
	t.mod, t.type = applytypedef(t.mod, ft)
 end

 if t.kind=="var" and (string.find(t.mod, "tolua_property%s") or string.find(t.mod, "tolua_property$")) then
 	t.mod = string.gsub(t.mod, "tolua_property", "tolua_property__"..get_property_type())
 end

 return t
end

-- Constructor
-- Expects the string declaration.
-- The kind of declaration can be "var" or "func".
function Declaration (s,kind,is_parameter)

 -- eliminate spaces if default value is provided
 s = gsub(s,"%s*=%s*","=")
 s = gsub(s, "%s*<", "<")

 local defb,tmpdef
 defb,_,tmpdef = string.find(s, "(=.*)$")
 if defb then
 	s = string.gsub(s, "=.*$", "")
 else
 	tmpdef = ''
 end
 if kind == "var" then
  -- check the form: void
  if s == '' or s == 'void' then
   return _Declaration{type = 'void', kind = kind, is_parameter = is_parameter}
  end
 end

 -- check the form: mod type*& name
 local t = split_c_tokens(s,'%*%s*&')
 if t.n == 2 then
  if kind == 'func' then
   error("#invalid function return type: "..s)
  end
  --local m = split(t[1],'%s%s*')
  local m = split_c_tokens(t[1],'%s+')
  return _Declaration{
   name = t[2]..tmpdef,
   ptr = '*',
   ret = '&',
   --type = rebuild_template(m[m.n], tb, timpl),
   type = m[m.n],
   mod = concat(m,1,m.n-1),
   is_parameter = is_parameter,
   kind = kind
  }
 end

 -- check the form: mod type** name
 t = split_c_tokens(s,'%*%s*%*')
 if t.n == 2 then
  if kind == 'func' then
   error("#invalid function return type: "..s)
  end
  --local m = split(t[1],'%s%s*')
  local m = split_c_tokens(t[1],'%s+')
  return _Declaration{
   name = t[2]..tmpdef,
   ptr = '*',
   ret = '*',
   --type = rebuild_template(m[m.n], tb, timpl),
   type = m[m.n],
   mod = concat(m,1,m.n-1),
   is_parameter = is_parameter,
   kind = kind
  }
 end

 -- check the form: mod type& name
 t = split_c_tokens(s,'&')
 if t.n == 2 then
  --local m = split(t[1],'%s%s*')
  local m = split_c_tokens(t[1],'%s+')
  return _Declaration{
   name = t[2]..tmpdef,
   ptr = '&',
   --type = rebuild_template(m[m.n], tb, timpl),
   type = m[m.n],
   mod = concat(m,1,m.n-1),
   is_parameter = is_parameter,
   kind = kind
  }
 end

 -- check the form: mod type* name
 local s1 = gsub(s,"(%b\[\])",function (n) return gsub(n,'%*','\1') end)
 t = split_c_tokens(s1,'%*')
 if t.n == 2 then
  t[2] = gsub(t[2],'\1','%*') -- restore * in dimension expression
  --local m = split(t[1],'%s%s*')
  local m = split_c_tokens(t[1],'%s+')
  return _Declaration{
   name = t[2]..tmpdef,
   ptr = '*',
   type = m[m.n],
   --type = rebuild_template(m[m.n], tb, timpl),
   mod = concat(m,1,m.n-1)   ,
   is_parameter = is_parameter,
   kind = kind
  }
 end

 if kind == 'var' then
  -- check the form: mod type name
  --t = split(s,'%s%s*')
  t = split_c_tokens(s,'%s+')
  local v
  if findtype(t[t.n]) then v = create_varname() else v = t[t.n]; t.n = t.n-1 end
  return _Declaration{
   name = v..tmpdef,
   --type = rebuild_template(t[t.n], tb, timpl),
   type = t[t.n],
   mod = concat(t,1,t.n-1),
   is_parameter = is_parameter,
   kind = kind
  }

 else -- kind == "func"

  -- check the form: mod type name
  --t = split(s,'%s%s*')
  t = split_c_tokens(s,'%s+')
  local v = t[t.n]  -- last word is the function name
  local tp,md
  if t.n>1 then
   tp = t[t.n-1]
   md = concat(t,1,t.n-2)
  end
  --if tp then tp = rebuild_template(tp, tb, timpl) end
  return _Declaration{
   name = v,
   type = tp,
   mod = md,
   is_parameter = is_parameter,
   kind = kind
  }
 end

end


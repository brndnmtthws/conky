-- tolua: operator class
-- Written by Waldemar Celes
-- TeCGraf/PUC-Rio
-- Jul 1998
-- $Id: $

-- This code is free software; you can redistribute it and/or modify it.
-- The software provided hereunder is on an "as is" basis, and
-- the author has no obligation to provide maintenance, support, updates,
-- enhancements, or modifications.


-- Operator class
-- Represents an operator function or a class operator method.
-- It stores the same fields as functions do plus:
--  kind = set of character representing the operator (as it appers in C++ code)
classOperator = {
 kind = '',
}
classOperator.__index = classOperator
setmetatable(classOperator,classFunction)

-- table to transform operator kind into the appropriate tag method name
_TM = {['+'] = 'add',
       ['-'] = 'sub',
       ['*'] = 'mul',
       ['/'] = 'div',
       ['<'] = 'lt',
       ['<='] = 'le',
       ['=='] = 'eq',
       ['[]'] = 'geti',
       ['&[]'] = 'seti',
       --['->'] = 'flechita',
      }


-- Print method
function classOperator:print (ident,close)
 print(ident.."Operator{")
 print(ident.." kind  = '"..self.kind.."',")
 print(ident.." mod  = '"..self.mod.."',")
 print(ident.." type = '"..self.type.."',")
 print(ident.." ptr  = '"..self.ptr.."',")
 print(ident.." name = '"..self.name.."',")
 print(ident.." const = '"..self.const.."',")
 print(ident.." cname = '"..self.cname.."',")
 print(ident.." lname = '"..self.lname.."',")
 print(ident.." args = {")
 local i=1
 while self.args[i] do
  self.args[i]:print(ident.."  ",",")
  i = i+1
 end
 print(ident.." }")
 print(ident.."}"..close)
end

function classOperator:supcode_tmp()

	if not _TM[self.kind] then
		return classFunction.supcode(self)
	end

	-- no overload, no parameters, always inclass
	output("/* method:",self.name," of class ",self:inclass()," */")

	output("#ifndef TOLUA_DISABLE_"..self.cname)
	output("\nstatic int",self.cname,"(lua_State* tolua_S)")

	if overload < 0 then
	 output('#ifndef TOLUA_RELEASE\n')
	end
	output(' tolua_Error tolua_err;')
	output(' if (\n')
	-- check self
	local is_func = get_is_function(self.parent.type)
	output('     !'..is_func..'(tolua_S,1,"'..self.parent.type..'",0,&tolua_err) ||\n')
	output('     !tolua_isnoobj(tolua_S,2,&tolua_err)\n )')
	output('  goto tolua_lerror;')

	output(' else\n')
	output('#endif\n') -- tolua_release
	output(' {')

	-- declare self
	output(' ',self.const,self.parent.type,'*','self = ')
	output('(',self.const,self.parent.type,'*) ')
	local to_func = get_to_func(self.parent.type)
	output(to_func,'(tolua_S,1,0);')

	-- check self
	output('#ifndef TOLUA_RELEASE\n')
	output('  if (!self) tolua_error(tolua_S,"'..output_error_hook("invalid \'self\' in function \'%s\'", self.name)..'",NULL);');
	output('#endif\n')

	-- cast self
	output('  ',self.mod,self.type,self.ptr,'tolua_ret = ')
	output('(',self.mod,self.type,self.ptr,')(*self);')

	-- return value
	local t,ct = isbasic(self.type)
	if t then
		output('   tolua_push'..t..'(tolua_S,(',ct,')tolua_ret);')
	else
		t = self.type
		local push_func = get_push_function(t)
		new_t = string.gsub(t, "const%s+", "")
		if self.ptr == '' then
			output('   {')
			output('#ifdef __cplusplus\n')
			output('    void* tolua_obj = Mtolua_new((',new_t,')(tolua_ret));')
			output('    ',push_func,'(tolua_S,tolua_obj,"',t,'");')
			output('    tolua_register_gc(tolua_S,lua_gettop(tolua_S));')
			output('#else\n')
			output('    void* tolua_obj = tolua_copy(tolua_S,(void*)&tolua_ret,sizeof(',t,'));')
			output('    ',push_func,'(tolua_S,tolua_obj,"',t,'");')
			output('    tolua_register_gc(tolua_S,lua_gettop(tolua_S));')
			output('#endif\n')
			output('   }')
		elseif self.ptr == '&' then
			output('   ',push_func,'(tolua_S,(void*)&tolua_ret,"',t,'");')
		else
			if local_constructor then
				output('   ',push_func,'(tolua_S,(void *)tolua_ret,"',t,'");')
				output('    tolua_register_gc(tolua_S,lua_gettop(tolua_S));')
			else
				output('   ',push_func,'(tolua_S,(void*)tolua_ret,"',t,'");')
			end
		end
	end

	output('  }')
	output(' return 1;')

	output('#ifndef TOLUA_RELEASE\n')
	output('tolua_lerror:\n')
	output(' tolua_error(tolua_S,"'..output_error_hook("#ferror in function \'%s\'.", self.lname)..'",&tolua_err);')
	output(' return 0;')
	output('#endif\n')


	output('}')
	output('#endif //#ifndef TOLUA_DISABLE\n')
	output('\n')
end

-- Internal constructor
function _Operator (t)
 setmetatable(t,classOperator)

 if t.const ~= 'const' and t.const ~= '' then
  error("#invalid 'const' specification")
 end

 append(t)
 if not t:inclass() then
  error("#operator can only be defined as class member")
 end

 --t.name = t.name .. "_" .. (_TM[t.kind] or t.kind)
 t.cname = t:cfuncname("tolua")..t:overload(t)
 t.name = "operator" .. t.kind  -- set appropriate calling name
 return t
end

-- Constructor
function Operator (d,k,a,c)

	local op_k = string.gsub(k, "^%s*", "")
	op_k = string.gsub(k, "%s*$", "")
	--if string.find(k, "^[%w_:%d<>%*%&]+$") then
	if d == "operator" and k ~= '' then

		d = k.." operator"
	elseif not _TM[op_k] then

		if flags['W'] then
			error("tolua: no support for operator" .. f.kind)
		else
			warning("No support for operator "..op_k..", ignoring")
			return nil
		end
	end

	local ref = ''
 local t = split_c_tokens(strsub(a,2,strlen(a)-1),',') -- eliminate braces
 local i=1
 local l = {n=0}
 while t[i] do
  l.n = l.n+1
  l[l.n] = Declaration(t[i],'var')
  i = i+1
 end
 if k == '[]' then
	 local _
	 _, _, ref = strfind(d,'(&)')
  d = gsub(d,'&','')
 elseif k=='&[]' then
  l.n = l.n+1
  l[l.n] = Declaration(d,'var')
  l[l.n].name = 'tolua_value'
 end
 local f = Declaration(d,'func')
 if k == '[]' and (l[1]==nil or isbasic(l[1].type)~='number') then
  error('operator[] can only be defined for numeric index.')
 end
 f.args = l
 f.const = c
 f.kind = op_k
 f.lname = "."..(_TM[f.kind] or f.kind)
 if not _TM[f.kind] then
 	f.cast_operator = true
 end
 if f.kind == '[]' and ref=='&' and f.const~='const' then
  Operator(d,'&'..k,a,c) 	-- create correspoding set operator
 end
 return _Operator(f)
end



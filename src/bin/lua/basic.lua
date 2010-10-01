-- tolua: basic utility functions
-- Written by Waldemar Celes
-- TeCGraf/PUC-Rio
-- Jul 1998
-- Last update: Apr 2003
-- $Id: $

-- This code is free software; you can redistribute it and/or modify it.
-- The software provided hereunder is on an "as is" basis, and
-- the author has no obligation to provide maintenance, support, updates,
-- enhancements, or modifications.


-- Basic C types and their corresponding Lua types
-- All occurrences of "char*" will be replaced by "_cstring",
-- and all occurrences of "void*" will be replaced by "_userdata"
_basic = {
 ['void'] = '',
 ['char'] = 'number',
 ['int'] = 'number',
 ['short'] = 'number',
 ['long'] = 'number',
 ['unsigned'] = 'number',
 ['float'] = 'number',
 ['double'] = 'number',
 ['_cstring'] = 'string',
 ['_userdata'] = 'userdata',
 ['char*'] = 'string',
 ['void*'] = 'userdata',
 ['bool'] = 'boolean',
 ['lua_Object'] = 'value',
 ['LUA_VALUE'] = 'value',    -- for compatibility with tolua 4.0
 ['lua_State*'] = 'state',
 ['_lstate'] = 'state',
 ['lua_Function'] = 'value',
}

_basic_ctype = {
 number = "lua_Number",
 string = "const char*",
 userdata = "void*",
 boolean = "bool",
 value = "int",
 state = "lua_State*",
}

-- functions the are used to do a 'raw push' of basic types
_basic_raw_push = {}

-- List of user defined types
-- Each type corresponds to a variable name that stores its tag value.
_usertype = {}

-- List of types that have to be collected
_collect = {}

-- List of types
_global_types = {n=0}
_global_types_hash = {}

-- list of classes
_global_classes = {}

-- List of enum constants
_global_enums = {}

-- List of auto renaming
_renaming = {}
function appendrenaming (s)
 local b,e,old,new = strfind(s,"%s*(.-)%s*@%s*(.-)%s*$")
	if not b then
	 error("#Invalid renaming syntax; it should be of the form: pattern@pattern")
	end
	tinsert(_renaming,{old=old, new=new})
end

function applyrenaming (s)
	for i=1,getn(_renaming) do
	 local m,n = gsub(s,_renaming[i].old,_renaming[i].new)
		if n ~= 0 then
		 return m
		end
	end
	return nil
end

-- Error handler
function tolua_error (s,f)
if _curr_code then
	print("***curr code for error is "..tostring(_curr_code))
	print(debug.traceback())
end
 local out = _OUTPUT
 _OUTPUT = _STDERR
 if strsub(s,1,1) == '#' then
  write("\n** tolua: "..strsub(s,2)..".\n\n")
  if _curr_code then
   local _,_,s = strfind(_curr_code,"^%s*(.-\n)") -- extract first line
   if s==nil then s = _curr_code end
   s = gsub(s,"_userdata","void*") -- return with 'void*'
   s = gsub(s,"_cstring","char*")  -- return with 'char*'
   s = gsub(s,"_lstate","lua_State*")  -- return with 'lua_State*'
   write("Code being processed:\n"..s.."\n")
  end
 else
 if not f then f = "(f is nil)" end
  print("\n** tolua internal error: "..f..s..".\n\n")
  return
 end
 _OUTPUT = out
end

function warning (msg)
 if flags.q then return end
 local out = _OUTPUT
 _OUTPUT = _STDERR
 write("\n** tolua warning: "..msg..".\n\n")
 _OUTPUT = out
end

-- register an user defined type: returns full type
function regtype (t)
	--if isbasic(t) then
	--	return t
	--end
	local ft = findtype(t)

	if not _usertype[ft] then
		return appendusertype(t)
	end
	return ft
end

-- return type name: returns full type
function typevar(type)
	if type == '' or type == 'void' then
		return type
	else
		local ft = findtype(type)
		if ft then
			return ft
		end
		_usertype[type] = type
		return type
	end
end

-- check if basic type
function isbasic (type)
 local t = gsub(type,'const ','')
 local m,t = applytypedef('', t)
 local b = _basic[t]
 if b then
  return b,_basic_ctype[b]
 end
 return nil
end

-- split string using a token
function split (s,t)
 local l = {n=0}
 local f = function (s)
  l.n = l.n + 1
  l[l.n] = s
  return ""
 end
 local p = "%s*(.-)%s*"..t.."%s*"
 s = gsub(s,"^%s+","")
 s = gsub(s,"%s+$","")
 s = gsub(s,p,f)
 l.n = l.n + 1
 l[l.n] = gsub(s,"(%s%s*)$","")
 return l
end

-- splits a string using a pattern, considering the spacial cases of C code (templates, function parameters, etc)
-- pattern can't contain the '^' (as used to identify the begining of the line)
-- also strips whitespace
function split_c_tokens(s, pat)

	s = string.gsub(s, "^%s*", "")
	s = string.gsub(s, "%s*$", "")

	local token_begin = 1
	local token_end = 1
	local ofs = 1
	local ret = {n=0}

	function add_token(ofs)

		local t = string.sub(s, token_begin, ofs)
		t = string.gsub(t, "^%s*", "")
		t = string.gsub(t, "%s*$", "")
		ret.n = ret.n + 1
		ret[ret.n] = t
	end

	while ofs <= string.len(s) do

		local sub = string.sub(s, ofs, -1)
		local b,e = string.find(sub, "^"..pat)
		if b then
			add_token(ofs-1)
			ofs = ofs+e
			token_begin = ofs
		else
			local char = string.sub(s, ofs, ofs)
			if char == "(" or char == "<" then

				local block
				if char == "(" then block = "^%b()" end
				if char == "<" then block = "^%b<>" end

				b,e = string.find(sub, block)
				if not b then
					-- unterminated block?
					ofs = ofs+1
				else
					ofs = ofs + e
				end

			else
				ofs = ofs+1
			end
		end

	end
	add_token(ofs)
	--if ret.n == 0 then

	--	ret.n=1
	--	ret[1] = ""
	--end

	return ret

end

-- concatenate strings of a table
function concat (t,f,l,jstr)
	jstr = jstr or " "
 local s = ''
 local i=f
 while i<=l do
  s = s..t[i]
  i = i+1
  if i <= l then s = s..jstr end
 end
 return s
end

-- concatenate all parameters, following output rules
function concatparam (line, ...)
 local i=1
 while i<=arg.n do
  if _cont and not strfind(_cont,'[%(,"]') and
     strfind(arg[i],"^[%a_~]") then
	    line = line .. ' '
  end
  line = line .. arg[i]
  if arg[i] ~= '' then
   _cont = strsub(arg[i],-1,-1)
  end
  i = i+1
 end
 if strfind(arg[arg.n],"[%/%)%;%{%}]$") then
  _cont=nil line = line .. '\n'
 end
	return line
end

-- output line
function output (...)
 local i=1
 while i<=arg.n do
  if _cont and not strfind(_cont,'[%(,"]') and
     strfind(arg[i],"^[%a_~]") then
	    write(' ')
  end
  write(arg[i])
  if arg[i] ~= '' then
   _cont = strsub(arg[i],-1,-1)
  end
  i = i+1
 end
 if strfind(arg[arg.n],"[%/%)%;%{%}]$") then
  _cont=nil write('\n')
 end
end

function get_property_methods(ptype, name)

	if get_property_methods_hook and get_property_methods_hook(ptype,name) then
		return get_property_methods_hook(ptype, name)
	end

	if ptype == "default" then -- get_name, set_name
		return "get_"..name, "set_"..name
	end

	if ptype == "qt" then -- name, setName
		return name, "set"..string.upper(string.sub(name, 1, 1))..string.sub(name, 2, -1)
	end

	if ptype == "overload" then -- name, name
		return name,name
	end

	return nil
end

-------------- the hooks

-- called right after processing the $[ichl]file directives,
-- right before processing anything else
-- takes the package object as the parameter
function preprocess_hook(p)
	-- p.code has all the input code from the pkg
end


-- called for every $ifile directive
-- takes a table with a string called 'code' inside, the filename, and any extra arguments
-- passed to $ifile. no return value
function include_file_hook(t, filename, ...)

end

-- called after processing anything that's not code (like '$renaming', comments, etc)
-- and right before parsing the actual code.
-- takes the Package object with all the code on the 'code' key. no return value
function preparse_hook(package)

end

-- called before starting output
function pre_output_hook(package)

end

-- called after writing all the output.
-- takes the Package object
function post_output_hook(package)

end


-- called from 'get_property_methods' to get the methods to retrieve a property
-- according to its type
function get_property_methods_hook(property_type, name)

end

-- called from ClassContainer:doparse with the string being parsed
-- return nil, or a substring
function parser_hook(s)

	return nil
end

-- called from classFunction:supcode, before the call to the function is output
function pre_call_hook(f)

end

-- called from classFunction:supcode, after the call to the function is output
function post_call_hook(f)

end

-- called before the register code is output
function pre_register_hook(package)

end

-- called to output an error message
function output_error_hook(...)
	return string.format(...)
end

-- custom pushers

_push_functions = {}
_is_functions = {}
_to_functions = {}

_base_push_functions = {}
_base_is_functions = {}
_base_to_functions = {}

local function search_base(t, funcs)

	local class = _global_classes[t]

	while class do
		if funcs[class.type] then
			return funcs[class.type]
		end
		class = _global_classes[class.btype]
	end
	return nil
end

function get_push_function(t)
	return _push_functions[t] or search_base(t, _base_push_functions) or "tolua_pushusertype"
end

function get_to_function(t)
	return _to_functions[t] or search_base(t, _base_to_functions) or "tolua_tousertype"
end

function get_is_function(t)
	return _is_functions[t] or search_base(t, _base_is_functions) or "tolua_isusertype"
end

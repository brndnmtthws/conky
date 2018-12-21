-- tolua: code class
-- Written by Waldemar Celes
-- TeCGraf/PUC-Rio
-- Jul 1999
-- $Id: $

-- This code is free software; you can redistribute it and/or modify it.
-- The software provided hereunder is on an "as is" basis, and
-- the author has no obligation to provide maintenance, support, updates,
-- enhancements, or modifications.

-- global
code_n = 1

-- Code class
-- Represents Lua code to be compiled and included
-- in the initialization function.
-- The following fields are stored:
--   text = text code
classCode = {
 text = '',
}
classCode.__index = classCode
setmetatable(classCode,classFeature)

-- register code
function classCode:register (pre)
 pre = pre or ''
 -- clean Lua code
 local s = clean(self.text)
 if not s then
  --print(self.text)
  error("parser error in embedded code")
 end

 -- get first line
 local _, _, first_line=string.find(self.text, "^([^\n\r]*)")
 if string.find(first_line, "^%s*%-%-") then
	 if string.find(first_line, "^%-%-##") then
		first_line = string.gsub(first_line, "^%-%-##", "")
		if flags['C'] then
			s = string.gsub(s, "^%-%-##[^\n\r]*\n", "")
		end
	 end
 else
 	first_line = ""
 end

 -- pad to 16 bytes
 local npad = 16 - (#s % 16)
 local spad = ""
 for i=1,npad do
 	spad = spad .. "-"
 end
 s = s..spad
 
 -- convert to C
 output('\n'..pre..'{ /* begin embedded lua code */\n')
 output(pre..' int top = lua_gettop(tolua_S);')
 output(pre..' static const unsigned char B[] = {\n   ')
 local t={n=0}

 local b = gsub(s,'(.)',function (c)
                         local e = ''
                         t.n=t.n+1 if t.n==15 then t.n=0 e='\n'..pre..'  ' end
                         return format('%3u,%s',strbyte(c),e)
                        end
               )
 output(b..strbyte(" "))
 output('\n'..pre..' };\n')
 if first_line and first_line ~= "" then
 	output(pre..' tolua_dobuffer(tolua_S,(char*)B,sizeof(B),"tolua embedded: '..first_line..'");')
 else
 	output(pre..' tolua_dobuffer(tolua_S,(char*)B,sizeof(B),"tolua: embedded Lua code '..code_n..'");')
 end
 output(pre..' lua_settop(tolua_S, top);')
 output(pre..'} /* end of embedded lua code */\n\n')
 code_n = code_n +1
end


-- Print method
function classCode:print (ident,close)
 print(ident.."Code{")
 print(ident.." text = [["..self.text.."]],")
 print(ident.."}"..close)
end


-- Internal constructor
function _Code (t)
 setmetatable(t,classCode)
 append(t)
 return t
end

-- Constructor
-- Expects a string representing the code text
function Code (l)
 return _Code {
  text = l
 }
end



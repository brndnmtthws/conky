-- tolua: module class
-- Written by Waldemar Celes
-- TeCGraf/PUC-Rio
-- Jul 1998
-- $Id: $

-- This code is free software; you can redistribute it and/or modify it.
-- The software provided hereunder is on an "as is" basis, and
-- the author has no obligation to provide maintenance, support, updates,
-- enhancements, or modifications.



-- Module class
-- Represents module.
-- The following fields are stored:
--    {i} = list of objects in the module.
classModule = {
 classtype = 'module'
}
classModule.__index = classModule
setmetatable(classModule,classContainer)

-- register module
function classModule:register (pre)
 pre = pre or ''
 push(self)
 output(pre..'tolua_module(tolua_S,"'..self.name..'",',self:hasvar(),');')
 output(pre..'tolua_beginmodule(tolua_S,"'..self.name..'");')
 local i=1
 while self[i] do
  self[i]:register(pre..' ')
  i = i+1
 end
 output(pre..'tolua_endmodule(tolua_S);')
	pop()
end

-- Print method
function classModule:print (ident,close)
 print(ident.."Module{")
 print(ident.." name = '"..self.name.."';")
 local i=1
 while self[i] do
  self[i]:print(ident.." ",",")
  i = i+1
 end
 print(ident.."}"..close)
end

-- Internal constructor
function _Module (t)
 setmetatable(t,classModule)
 append(t)
 return t
end

-- Constructor
-- Expects two string representing the module name and body.
function Module (n,b)
 local t = _Module(_Container{name=n})
 push(t)
 t:parse(strsub(b,2,strlen(b)-1)) -- eliminate braces
 pop()
 return t
end



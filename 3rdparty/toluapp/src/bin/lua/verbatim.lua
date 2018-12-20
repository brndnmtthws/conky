-- tolua: verbatim class
-- Written by Waldemar Celes
-- TeCGraf/PUC-Rio
-- Jul 1998
-- $Id: verbatim.lua,v 1.3 2000/01/24 20:41:16 celes Exp $

-- This code is free software; you can redistribute it and/or modify it.
-- The software provided hereunder is on an "as is" basis, and
-- the author has no obligation to provide maintenance, support, updates,
-- enhancements, or modifications.



-- Verbatim class
-- Represents a line translated directed to the binding file.
-- The following filds are stored:
--   line = line text
classVerbatim = {
 line = '',
	cond = nil,    -- condition: where to generate the code (s=suport, r=register)
}
classVerbatim.__index = classVerbatim
setmetatable(classVerbatim,classFeature)

-- preamble verbatim
function classVerbatim:preamble ()
 if self.cond == '' then
  write(self.line)
 end
end

-- support code
function classVerbatim:supcode ()
 if strfind(self.cond,'s') then
  write(self.line)
  write('\n')
 end
end

-- register code
function classVerbatim:register (pre)
 if strfind(self.cond,'r') then
  write(self.line)
 end
end


-- Print method
function classVerbatim:print (ident,close)
 print(ident.."Verbatim{")
 print(ident.." line = '"..self.line.."',")
 print(ident.."}"..close)
end


-- Internal constructor
function _Verbatim (t)
 setmetatable(t,classVerbatim)
 append(t)
 return t
end

-- Constructor
-- Expects a string representing the text line
function Verbatim (l,cond)
 if strsub(l,1,1) == "'" then
  l = strsub(l,2)
 elseif strsub(l,1,1) == '$' then
  cond = 'sr'       -- generates in both suport and register fragments
  l = strsub(l,2)
 end
 return _Verbatim {
  line = l,
  cond = cond or '',
 }
end



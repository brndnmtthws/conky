-- tolua: abstract feature class
-- Written by Waldemar Celes
-- TeCGraf/PUC-Rio
-- Jul 1998
-- $Id: $

-- This code is free software; you can redistribute it and/or modify it.
-- The software provided hereunder is on an "as is" basis, and
-- the author has no obligation to provide maintenance, support, updates,
-- enhancements, or modifications.


-- Feature class
-- Represents the base class of all mapped feature.
classFeature = {
}
classFeature.__index = classFeature

-- write support code
function classFeature:supcode ()
end

-- output tag
function classFeature:decltype ()
end

-- register feature
function classFeature:register (pre)
end

-- translate verbatim
function classFeature:preamble ()
end

-- check if it is a variable
function classFeature:isvariable ()
 return false
end

-- check if it requires collection
function classFeature:requirecollection (t)
 return false
end

-- build names
function classFeature:buildnames ()
 if self.name and self.name~='' then
  local n = split(self.name,'@')
  self.name = n[1]
  self.name = string.gsub(self.name, ":%d*$", "")
  if not n[2] then
   n[2] = applyrenaming(n[1])
  end
  self.lname = n[2] or gsub(n[1],"%[.-%]","")
  self.lname = string.gsub(self.lname, ":%d*$", "")
  self.original_name = self.name
  self.lname = clean_template(self.lname)
 end
 if not self.is_parameter then
	 self.name = getonlynamespace() .. self.name
 end

 local parent = classContainer.curr
 if parent then
 	self.access = parent.curr_member_access
	self.global_access = self:check_public_access()
 else
 end
end

function classFeature:check_public_access()

	if type(self.global_access) == "boolean" then
		return self.global_access
	end

	if self.access and self.access ~= 0 then
		return false
	end

	local parent = classContainer.curr
	while parent do
		if parent.access and parent.access ~= 0 then
			return false
		end
		parent = parent.prox
	end
	return true
end

function clean_template(t)

	return string.gsub(t, "[<>:, %*]", "_")
end

-- check if feature is inside a container definition
-- it returns the container class name or nil.
function classFeature:incontainer (which)
 if self.parent then
  local parent = self.parent
  while parent do
   if parent.classtype == which then
    return parent.name
   end
   parent = parent.parent
  end
 end
 return nil
end

function classFeature:inclass ()
 return self:incontainer('class')
end

function classFeature:inmodule ()
 return self:incontainer('module')
end

function classFeature:innamespace ()
 return self:incontainer('namespace')
end

-- return C binding function name based on name
-- the client specifies a prefix
function classFeature:cfuncname (n)

 if self.parent then
  n = self.parent:cfuncname(n)
 end

 local fname = self.lname
 if not fname or fname == '' then
 	fname = self.name
 end
  n = string.gsub(n..'_'.. (fname), "[<>:, \.%*&]", "_")

  return n
end


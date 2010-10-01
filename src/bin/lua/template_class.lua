
_global_templates = {}

classTemplateClass = {

	name = '',
	body = '',
	parents = {},
	args = {}, -- the template arguments
}

classTemplateClass.__index = classTemplateClass


function classTemplateClass:throw(types, local_scope)

	--if table.getn(types) ~= table.getn(self.args) then
	--	error("#invalid parameter count")
	--end

	-- replace
	for i =1 , types.n do

		local Il = split_c_tokens(types[i], " ")
		if table.getn(Il) ~= table.getn(self.args) then
			error("#invalid parameter count for "..types[i])
		end
		local bI = self.body
		local pI = {}
		for j = 1,self.args.n do
			--Tl[j] = findtype(Tl[j]) or Tl[j]
			bI = string.gsub(bI, "([^_%w])"..self.args[j].."([^_%w])", "%1"..Il[j].."%2")
			if self.parents then
				for i=1,table.getn(self.parents) do
					pI[i] = string.gsub(self.parents[i], "([^_%w]?)"..self.args[j].."([^_%w]?)", "%1"..Il[j].."%2")
				end
			end
		end
		--local append = "<"..string.gsub(types[i], "%s+", ",")..">"
		local append = "<"..concat(Il, 1, table.getn(Il), ",")..">"
		append = string.gsub(append, "%s*,%s*", ",")
		append = string.gsub(append, ">>", "> >")
		for i=1,table.getn(pI) do
			--pI[i] = string.gsub(pI[i], ">>", "> >")
			pI[i] = resolve_template_types(pI[i])
		end
		bI = string.gsub(bI, ">>", "> >")
		local n = self.name
		if local_scope then
			n = self.local_name
		end

		Class(n..append, pI, bI)
	end
end


function TemplateClass(name, parents, body, parameters)

	local o = {
	
		parents = parents,
		body = body,
		args = parameters,
	}
	
	local oname = string.gsub(name, "@.*$", "")
	oname = getnamespace(classContainer.curr)..oname
	o.name = oname

	o.local_name = name
	
	setmetatable(o, classTemplateClass)

	if _global_templates[oname] then
		warning("Duplicate declaration of template "..oname)
	else
		_global_templates[oname] = o
	end

	return o
end

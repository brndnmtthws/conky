-- Generate binding code
-- Written by Waldemar Celes
-- TeCGraf/PUC-Rio
-- Jul 1998
-- Last update: Apr 2003
-- $Id: $


-- This code is free software; you can redistribute it and/or modify it.
-- The software provided hereunder is on an "as is" basis, and
-- the author has no obligation to provide maintenance, support, updates,
-- enhancements, or modifications.

function parse_extra()

	for k,v in ipairs(_extra_parameters or {}) do
		
		local b,e,name,value = string.find(v, "^([^=]*)=(.*)$")
		if b then
			_extra_parameters[name] = value
		else
			_extra_parameters[v] = true
		end
	end
end

function doit ()
	-- define package name, if not provided
	if not flags.n then
		if flags.f then
			flags.n = gsub(flags.f,"%..*$","")
			_,_,flags.n = string.find(flags.n, "([^/\\]*)$")
		else
			error("#no package name nor input file provided")
		end
	end

	-- parse table with extra paramters
	parse_extra()

	-- do this after setting the package name
	if flags['L'] then
		dofile(flags['L'])
	end

	-- add cppstring
	if not flags['S'] then
		_basic['string'] = 'cppstring'
		_basic['std::string'] = 'cppstring'
		_basic_ctype.cppstring = 'const char*'
	end

	-- proccess package
	local p  = Package(flags.n,flags.f)

	if flags.p then
		return        -- only parse
	end

	if flags.o then
		local st,msg = writeto(flags.o)
		if not st then
			error('#'..msg)
		end
	end

	p:decltype()
	if flags.P then
		p:print()
	else
		push(p)
		pre_output_hook(p)
		pop()
		p:preamble()
		p:supcode()
		push(p)
		pre_register_hook(p)
		pop()
		p:register()
		push(p)
		post_output_hook(p)
		pop()
	end

	if flags.o then
		writeto()
	end

	-- write header file
	if not flags.P then
		if flags.H then
			local st,msg = writeto(flags.H)
			if not st then
				error('#'..msg)
			end
			p:header()
			writeto()
		end
	end
end


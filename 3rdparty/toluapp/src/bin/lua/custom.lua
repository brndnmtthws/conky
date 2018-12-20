
function extract_code(fn,s)
	local code = ""
	if fn then
		code = '\n$#include "'..fn..'"\n'
	end
	s= "\n" .. s .. "\n" -- add blank lines as sentinels
	local _,e,c,t = strfind(s, "\n([^\n]-)SCRIPT_([%w_]*)[^\n]*\n")
	while e do
		t = strlower(t)
		if t == "bind_begin" then
			_,e,c = strfind(s,"(.-)\n[^\n]*SCRIPT_BIND_END[^\n]*\n",e)
			if not e then
			 tolua_error("Unbalanced 'SCRIPT_BIND_BEGIN' directive in header file")
			end
		end
		if t == "bind_class" or t == "bind_block" then
			local b
			_,e,c,b = string.find(s, "([^{]-)(%b{})", e)
			c = c..'{\n'..extract_code(nil, b)..'\n};\n'
		end
		code = code .. c .. "\n"
	 _,e,c,t = strfind(s, "\n([^\n]-)SCRIPT_([%w_]*)[^\n]*\n",e)
	end
	return code
end

function preprocess_hook(p)
end

function preparse_hook(p)
end

function include_file_hook(p, filename)
	do return end
--print("FILENAME is "..filename)
	p.code = string.gsub(p.code, "\n%s*SigC::Signal", "\n\ttolua_readonly SigC::Signal")
	p.code = string.gsub(p.code, "#ifdef __cplusplus\nextern \"C\" {\n#endif", "")
	p.code = string.gsub(p.code, "#ifdef __cplusplus\n};?\n#endif", "")
	p.code = string.gsub(p.code, "DECLSPEC", "")
	p.code = string.gsub(p.code, "SDLCALL", "")
	p.code = string.gsub(p.code, "DLLINTERFACE", "")
	p.code = string.gsub(p.code, "#define[^\n]*_[hH]_?%s*\n", "\n")
--print("code is "..p.code)
end

-- mark up comments and strings
STR1 = "\001"
STR2 = "\002"
STR3 = "\003"
STR4 = "\004"
REM  = "\005"
ANY  = "([\001-\005])"
ESC1 = "\006"
ESC2 = "\007"

MASK = { -- the substitution order is important
 {ESC1, "\\'"},
 {ESC2, '\\"'},
 {STR1, "'"},
 {STR2, '"'},
 {STR3, "%[%["},
 {STR4, "%]%]"},
 {REM , "%-%-"},
}

function mask (s)
 for i = 1,getn(MASK)  do
  s = gsub(s,MASK[i][2],MASK[i][1])
 end
 return s
end

function unmask (s)
 for i = 1,getn(MASK)  do
  s = gsub(s,MASK[i][1],MASK[i][2])
 end
 return s
end

function clean (s)
 -- check for compilation error
 local code = "return function ()\n" .. s .. "\n end"
 if not dostring(code) then
  return nil
 end

 if flags['C'] then
 	return s
 end

 local S = "" -- saved string

 s = mask(s)

 -- remove blanks and comments
 while 1 do
  local b,e,d = strfind(s,ANY)
  if b then
   S = S..strsub(s,1,b-1)
   s = strsub(s,b+1)
   if d==STR1 or d==STR2 then
    e = strfind(s,d)
    S = S ..d..strsub(s,1,e)
    s = strsub(s,e+1)
   elseif d==STR3 then
    e = strfind(s,STR4)
    S = S..d..strsub(s,1,e)
    s = strsub(s,e+1)
   elseif d==REM then
    s = gsub(s,"[^\n]*(\n?)","%1",1)
   end
  else
   S = S..s
   break
  end
 end
 -- eliminate unecessary spaces
 S = gsub(S,"[ \t]+"," ")
 S = gsub(S,"[ \t]*\n[ \t]*","\n")
	S = gsub(S,"\n+","\n")
 S = unmask(S)
 return S
end


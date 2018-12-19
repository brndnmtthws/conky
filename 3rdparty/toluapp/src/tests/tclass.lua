if not Test then
	local loadlib
	if not package then
		loadlib = _G['loadlib']
	else
		loadlib = package.loadlib
	end
	f, e, eo = loadlib("./libtclass.so", "luaopen_tclass")
	if f then
		f()
	else
		print(eo, e)
		os.exit()
	end
end

a = {}
rawset(a, ".c_instance", "something")

function hello()

	print("hello world")
end

rawset(Test.B, "hello", hello)

-- type convertion tests
--print(Test.A)
--print(tolua.type(Test.A.last))
--assert(tolua.type(Test.A.last) == 'Test::Tst_A') -- first time the object is mapped
--assert(tolua.type(Test.B.last) == 'Test::Tst_B') -- type convertion to specialized type
--assert(tolua.type(Test.A.last) == 'Test::Tst_B') -- no convertion: obj already mapped as B


local a = Test.A:new()
assert(tolua.type(Test.A.last) == 'Test::Tst_A') -- no type convertion: same type
local b = Test.B:new()
assert(tolua.type(Test.A.last) == 'Test::Tst_B') -- no convertion: obj already mapped as B
local c = Test.luaC:new(0)
assert(tolua.type(Test.A.last) == 'Test::Tst_C') -- no convertion: obj already mapped as C
assert(tolua.type(Test.luaC.last) == 'Test::Tst_C')

local aa = Test.A.AA:new()
local bb = Test.A.BB:new()
local xx = Test.create_aa()

-- method calling tests
assert(a:a() == 'A')
assert(b:a() == 'A')
assert(b:b() == 'B')
assert(c:a() == 'A')
assert(c:b() == 'B')
assert(c:c() == 'C')
assert(aa:aa() == 'AA')
assert(bb:aa() == bb:Base():aa())
assert(xx:aa() == 'AA')
assert(Test.is_aa(bb) == true)

-- test ownershipping handling
-- should delete objects: 6 7 8 9 10 (it may vary!)
local set = {}
for i=1,10 do
 local c = Test.luaC:new(i)
	if i>5 then
		tolua.takeownership(c)
	end
	--set[i] = c
end



e = Test.B:new_local()

print("e is type "..tolua.type(e))
print("ae is type "..tolua.type(ae))

--e:delete()

b:hello()

----------
local out = Test.Outside:new_local()
out:outside()
Test.Outside:outside_static(nil)


print "***** cast"
local acast = Test.B:create_void()
print("type is "..tolua.type(acast))
local bcast = tolua.cast(acast, "Test::Tst_B")
print("bcast is "..tostring(bcast))
print("type is "..tolua.type(bcast))
print(bcast:b())

-- test properies
local n = 7
a.number = n
assert(a.number == n*2)

-- constructors
print(getmetatable(Test.A))
print(getmetatable(Test.B))
print(getmetatable(Test.E))

local a = Test.A()
local b = Test.B()
local e = Test.E(5)
--print(e+5)
print(tostring(getmetatable(Test.B).__call))
print(tostring(Test.B.__call))
print(tostring(Test.B.__call(Test.B)))
print(tolua.type(b))

e:set_ptr(e)
local ve = tolua.cast(e:get_ptr(), "Test::Tst_E")
ve:set_ptr(ve)

print"1"
Test.A.pete = {}
print"2"
table.insert(Test.A.pete, a)
print"3"


for i=1,100000 do
	la = {}
	tolua.inherit(la, a)
end

print("Class test OK")


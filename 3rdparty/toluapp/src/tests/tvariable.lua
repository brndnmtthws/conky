assert(i==1)
assert(f==2)
assert(d==3)
assert(s=="Hello world")
assert(n=="Hi there")
n = "Hello"
assert(n=="Hello")

assert(a.i==11)
assert(a.f==12)
assert(a.d==13)
assert(a.s=="Hello world from class")
assert(a.n=="Hi there from class")
a.n = "Hello from class"
assert(a.n=="Hello from class")

assert(v==a.v)

u.i = 2
assert(u.i==2)
u.f = 2
assert(u.f==2)
assert(u.i~=2)

assert(M.mi==21)
assert(M.mf==22)
assert(M.md==23)
assert(M.ms=="Hello world in module")
assert(M.mn=="Hi there in module")
M.mn = "Hello in module"
assert(M.mn=="Hello in module")
assert(M.mv==nil)

assert(M.ma.i==31)
assert(M.ma.f==32)
assert(M.ma.d==33)
assert(M.ma.s=="Hello world from class in module")
assert(M.ma.n=="Hi there from class in module")
M.ma.n = "Hello from class in module"
assert(M.ma.n=="Hello from class in module")
assert(M.ma.v==nil)

assert(a.i==b.a.i)
assert(a.f==b.a.f)
assert(a.d==b.a.d)
assert(a.s==b.a.s)
assert(a.v==b.a.v)
assert(b.b==nil)

assert(M.ma.i==M.mb.a.i)
assert(M.ma.f==M.mb.a.f)
assert(M.ma.d==M.mb.a.d)
assert(M.ma.s==M.mb.a.s)
assert(M.ma.v==M.mb.a.v)

assert(a.i==M.mb.b.a.i)
assert(a.f==M.mb.b.a.f)
assert(a.d==M.mb.b.a.d)
assert(a.s==M.mb.b.a.s)
assert(a.v==M.mb.b.a.v)
assert(M.mb.b.b==nil)

assert(s~=rawget(_G,"s"))  -- because s represents a C variable
s = "Hello"
assert(s==rawget(_G,"s"))  -- because s is mapped as const

f = 25.0
assert(f~=rawget(_G,"f"))  -- because f represents a C variable

b.a.i = 5
assert(b.a.i==M.mb.b.a.i)

print("Variable test OK")

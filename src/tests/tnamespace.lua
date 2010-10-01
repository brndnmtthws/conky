-- test valid access
assert(A.a==1)
assert(A.B.b==2)
assert(A.B.C.c==3)

-- test invalid access
assert(A.B.a==nil)      -- no inheritance
assert(A.B.C.a==nil)

assert(A.b==nil)        -- no access the inner module
assert(A.c==nil)
assert(A.B.c==nil)

print("Namespace test OK")

for i=1,10 do
 assert(a[i]==i)
end

for i=2,10 do
 assert(p[i-1].y==p[i].x)
end
 
for i=1,10 do
 assert(M.a[i]==i)
end

for i=2,10 do
 assert(M.p[i-1].y==M.p[i].x)
end

for i=1,10 do
 assert(pp[i].x==M.p[i].x and p[i].y == M.pp[i].y)
end

for i=1,10 do
 assert(array.a[i] == parray.a[i])
 assert(array.p[i].x == parray.pp[i].x and array.p[i].y == parray.pp[i].y)
end

for i=1,10 do
 array.a[i] = a[10-i+1]
	M.a[i] = 10-i+1
	assert(array.a[i]==M.a[i])
end

for i=2,10 do
 array.p[i] = array.pp[1]
	assert(array.p[i].x==0 and array.p[i].y==1)
end

print("Array test OK")

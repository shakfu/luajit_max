local ffi = require 'ffi'


ffi.cdef[[
inline int add(int x, int y)
{
	return x+y;
}
]]

print(ffi.add(1,2))

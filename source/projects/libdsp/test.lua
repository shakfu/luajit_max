local ffi = require 'ffi'

local dsp = ffi.load"./libdsp.dylib"

ffi.cdef[[
double scale_linear(double x, double in_min, double in_max, double out_min, double out_max);
double scale_sine1(double x, double in_min, double in_max, double out_min, double out_max);
double scale_sine2(double x, double in_min, double in_max, double out_min, double out_max);
double scale_exp1(double x, double s, double in_min, double in_max, double out_min, double out_max);
double scale_exp2(double x, double s, double in_min, double in_max, double out_min, double out_max);
double scale_log1(double x, double p, double i_min, double i_max, double o_min, double o_max);
double scale_log2(double x, double p, double i_min, double i_max, double o_min, double o_max);
]]



--                     		x     of   i_min   i_max   o_min  o_max
print( dsp.scale_linear(	50,        1,      127,    1,     100))
print(  dsp.scale_sine1( 	50,        1,      127,    1,     100))
print(  dsp.scale_sine2( 	50,        1,      127,    1,     100))
print(   dsp.scale_exp1(  	50,   2,   1,      127,    1,     100))
print(   dsp.scale_exp2(  	50,   2,   1,      127,    1,     100))
print(   dsp.scale_log1(  	50,   2,   1,      127,    1,     100))
print(   dsp.scale_log2(  	50,   2,   1,      127,    1,     100))


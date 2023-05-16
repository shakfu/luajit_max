
local sine = stk.SineWave()
sine:setFrequency(220);
for i = 1,10000 do
	sine:tick()
	if (i == 9999) then
		print("sine: " .. sine:tick())
	end
end


local p = app.Person("sam", 21)
local s = p:identify()
assert(s == "sam 21")
print(s)




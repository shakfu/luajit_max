-- dsp_worp.lua
-- extracted from worp dsp library
-- see: https://github.com/zevv/worp and http://worp.zevv.nl/index.html


getmetatable("").__mod = function(a, b)
   if not b then
      return a
   elseif type(b) == "table" then
      return string.format(a, unpack(b))
   else
      return string.format(a, b)
   end
end


srate = 44100

Dsp = {}

function Dsp:Mod(def, init)

	local mod = {

		-- methods
		
		update = function(mod)
			if def.controls.fn_update then
				def.controls.fn_update()
			end
		end,

		set = function(mod, id, value)
			if type(id) == "table" then
				for id, value in pairs(id) do
					mod:control(id):set(value, false)
				end
				mod:update()
			else
				mod:control(id):set(value)
			end
		end,

		controls = function(mod)
			return mod.control_list
		end,

		get = function(mod)
			return mod
		end,

		control = function(mod, id)
			return mod.control_list[id]
		end,

		help = function(mod)
			print("%s:" % { mod.description })
			for _, control in ipairs(mod:controls()) do
				local range = ""
				if #control.options > 0 then
					range = table.concat(control.options, "/")
				elseif control.min or control.max then
					range = "%s..%s" % { control.min or 0, control.max or 1 }
				end
				print(" - %s: %s (%s)" % { control.id, control.description, range })
			end
		end,

		-- data

		id = def.id,
		description = def.description,
		control_list = {}
	}
	
	setmetatable(mod, {
		__tostring = function()
			return "mod: %s" % def.description
		end,
		__call = function(_, ...)
			return def.fn_gen(...)
		end,
	})

	for i, def in ipairs(def.controls) do
		local control = Dsp:Control(def, mod)
		mod.control_list[i] = control
		mod.control_list[control.id] = control
		if init then
			control:set(init[control.id], false)
		end
	end

	mod:update()

	return mod
end

-- vi: ft=lua ts=3 sw=3



--
-- Attack / Decay / Sustain / Release module.
--
-- This module generates an envelope amplitude between 0.0 and 1.0. When the
-- 'vel' argument is set to >0 the envelope generator will start (note on),
-- when 'vel' is set to zero, the generator will go to the decay phase and fall
-- down to zero amplitude (note off)
--

function Dsp:Adsr(init)

	local arg = {
		A = 0,
		D = 0,
		S = 1,
		R = 0,
		on = true,
	}

	local state, v = nil, 0
	local velocity = 0
	local dv_A, dv_D, dv_R, level_S = 0, 0, 0, 1
	local dv = 0

	return Dsp:Mod({
		description = "ADSR envelope generator",
		controls = {
			{
				id = "vel",
				description = "Velocity",
				fn_set = function(val)
					if val > 0 then
						velocity = val
						state, dv = "A", dv_A
					end
					if val == 0 then
						state, dv = "R", dv_R
					end
				end
			}, {
				id = "A",
				description = "Attack",
				max = 10,
				unit = "s",
				default = 0.1,
				fn_set = function(val)
					dv_A =  math.min(1/(srate * val), 1)
				end,
			}, {
				id = "D",
				description = "Decay",
				max = 10,
				unit = "s",
				default = 0.1,
				fn_set = function(val)
					dv_D = math.max(-1/(srate * val), -1)
				end,
			}, {
				id = "S",
				description = "Sustain",
				default = 1,
				fn_set = function(val)
					level_S = val
				end
			}, {
				id = "R",
				description = "Release",
				max = 10,
				unit = "s",
				default = 0.1,
				fn_set = function(val)
					dv_R = math.max(-1/(srate * val), -1)
				end
			}, 
		},
		fn_gen = function()
			if state == "A" and v >= 1 then
				state, dv = "D", dv_D
			elseif state == "D" and v < level_S then
				state, dv = "S", 0
			end
			v = v + dv
			v = math.max(v, 0)
			v = math.min(v, 1)
			return v * velocity
		end,

	}, init)
end

-- vi: ft=lua ts=3 sw=3


--
-- Generator which outputs a constant value in the range 0..1, controlled by
-- the 'c' control. Useful for easy mapping of a GUI knob or midi CC to a value.
--

function Dsp:Const(init)

	local c

	return Dsp:Mod({
		description = "Const",
		controls = {
			{
				id = "c",
				description = "Value",
				default = 1,
				fmt = "%0.2f",
				fn_set = function(val) c = val end,
			},
		},

		fn_gen = function()
			return c
		end

	}, init)

end

-- vi: ft=lua ts=3 sw=3


function Dsp:Control(def, mod)

	local control = {

		-- methods

		get = function(control)
			return control.value
		end,
		
		set = function(control, value, update)
			if control.type == "enum" and type(value) == "number" then
				value = control.options[math.floor(value + 0.5)]
			end
			if value ~= nil then
				control.value = value
				for fn in pairs(control.fn_set) do
					fn(value)
				end
				if update ~= false then
					mod:update()
				end
			end
		end,

		set_uni = function(control, v, update)
			if control.min and control.max then
				v = control.min + v * (control.max-control.min)
				if control.log then v = (control.max+1) ^ (v/control.max) - 1 end
				control:set(v)
			end
		end,

		on_set = function(control, fn)
			control.fn_set[fn] = true
		end,

		-- data
	
		id = def.id or "",
		description = def.description or "",
		type = def.type or "number",
		fmt = def.fmt,
		min = def.min or 0,
		max = def.max or 1,
		options = def.options or {},
		log = def.log,
		unit = def.unit,
		default = def.default or 0,
		value = nil,
		fn_set = {},
	}

	if def.type == "enum" then
		control.min, control.max = 1, #def.options
	end

	setmetatable(control, {
		__tostring = function()
			return "control:%s(%s)" % { control.id, control.value }
		end
	})

	if def.fn_set then
		control:on_set(def.fn_set)
	end
	control:set(def.default, false)

	return control

end


-- vi: ft=lua ts=3 sw=3


-- 
-- The Filter module is a basic audio filter with configurable frequency, resonance and gain.
-- A number of different filter types are provided:
--
-- * hp: High pass
-- * lp: Low pass
-- * bp: Band pass
-- * bs: Band stop (aka, Notch)
-- * ls: Low shelf
-- * hs: High shelf
-- * ap: All pass
-- * eq: Peaking EQ filter
--
-- The code is based on a document from Robert Bristow-Johnson, check the original
-- at [http://www.musicdsp.org/files/Audio-EQ-Cookbook.txt] for more details
-- about the filter implementation
--

function Dsp:Filter(init)

	local fs = 44100
	local a0, a1, a2, b0, b1, b2
	local x0, x1, x2 = 0, 0, 0
	local y0, y1, y2 = 0, 0, 0

	local type, f, Q, gain

	return Dsp:Mod({
		description = "Biquad filter",
		controls = {
			fn_update = function()
				local w0 = 2 * math.pi * (f / fs)
				local alpha = math.sin(w0) / (2*Q)
				local cos_w0 = math.cos(w0)
				local A = math.pow(10, gain/40)

				if type == "hp" then
					b0, b1, b2 = (1 + cos_w0)/2, -(1 + cos_w0), (1 + cos_w0)/2
					a0, a1, a2 = 1 + alpha, -2*cos_w0, 1 - alpha

				elseif type == "lp" then
					b0, b1, b2 = (1 - cos_w0)/2, 1 - cos_w0, (1 - cos_w0)/2
					a0, a1, a2 = 1 + alpha, -2*cos_w0, 1 - alpha

				elseif type == "bp" then
					b0, b1, b2 = Q*alpha, 0, -Q*alpha
					a0, a1, a2 = 1 + alpha, -2*cos_w0, 1 - alpha

				elseif type == "bs" then
					b0, b1, b2 = 1, -2*cos_w0, 1
					a0, a1, a2 = 1 + alpha, -2*cos_w0, 1 - alpha

				elseif type == "ls" then
					local ap1, am1, tsAa = A+1, A-1, 2 * math.sqrt(A) * alpha
					local am1_cos_w0, ap1_cos_w0 = am1 * cos_w0, ap1 * cos_w0
					b0, b1, b2 = A*( ap1 - am1_cos_w0 + tsAa ), 2*A*( am1 - ap1_cos_w0 ), A*( ap1 - am1_cos_w0 - tsAa )
					a0, a1, a2 = ap1 + am1_cos_w0 + tsAa, -2*( am1 + ap1_cos_w0 ), ap1 + am1_cos_w0 - tsAa

				elseif type == "hs" then
					local ap1, am1, tsAa = A+1, A-1, 2 * math.sqrt(A) * alpha
					local am1_cos_w0, ap1_cos_w0 = am1 * cos_w0, ap1 * cos_w0
					b0, b1, b2 = A*( ap1 + am1_cos_w0 + tsAa ), -2*A*( am1 + ap1_cos_w0 ), A*( ap1 + am1_cos_w0 - tsAa )
					a0, a1, a2 = ap1 - am1_cos_w0 + tsAa, 2*( am1 - ap1_cos_w0 ), ap1 - am1_cos_w0 - tsAa

				elseif type == "eq" then
					b0, b1, b2 = 1 + alpha*A, -2*cos_w0, 1 - alpha*A
					a0, a1, a2 = 1 + alpha/A, -2*cos_w0, 1 - alpha/A

				elseif type == "ap" then
					b0, b1, b2 = 1 - alpha, -2*cos_w0, 1 + alpha
					a0, a1, a2 = 1 + alpha, -2*cos_w0, 1 - alpha

				else
					error("Unsupported filter type " .. type)
				end
			end,
			{
				id = "type",
				description = "Filter type",
				type = "enum",
				options =  { "lp", "hp", "bp", "bs", "ls", "hs", "eq", "ap" },
				default = "lp",
				fn_set = function(val) type = val end
			}, {
				id = "f",
				description = "Frequency",
				max = 20000,
				log = true,
				unit = "Hz",
				default = 440,
				fn_set = function(val) f = val end
			}, {
				id = "Q",
				description = "Resonance",
				min = 0.1,
				max = 100,
				log = true,
				default = 1,
				fn_set = function(val) Q = val end
			}, {
				id = "gain",
				description = "Shelf/EQ filter gain",
				min = -60,
				max = 60,
				unit = "dB",
				default = 0,
				fn_set = function(val) gain = val end
			}
		},

		fn_gen = function(x0)
			y2, y1 = y1, y0
			y0 = (b0 / a0) * x0 + (b1 / a0) * x1 + (b2 / a0) * x2 - (a1 / a0) * y1 - (a2 / a0) * y2
			x2, x1 = x1, x0
			return y0
		end
	}, init)

end

-- vi: ft=lua ts=3 sw=3


--
-- Random noise generator module, generates noise in the range -1.0 .. +1.0
--
-- The noise module can generate both uniform and gaussian white noise.
--
-- The gaussian noise is based on code from ..., check the original document at
-- [http://www.taygeta.com/random/gaussian.html] for more details about the
-- implementation
--

function Dsp:Noise(init)

	local random, sqrt, log = math.random, math.sqrt, math.log
	local type, y1, y2
	
	local function rand()
		return 2 * random() - 1
	end
	
	return Dsp:Mod({
		description = "Noise generator",
		controls = {
			{
				id = "type",
				description = "Noise type",
				type = "enum",
				options = { "uniform", "gaussian" },
				default = "uniform",
				fn_set = function(val) type = val end
			},
		},

		fn_gen = function()

			if type == "uniform" then
				return rand()
			end
		
			if type == "gaussian" then

				local x1, x2, w
				repeat
					x1, x2 = rand(), rand()
					w = x1 * x1 + x2 * x2
				until w < 1
				w = sqrt((-2 * log(w)) / w)
				y1 = x1 * w
				y2 = x2 * w
				return y1
			end
		end,

	}, init)

end

-- vi: ft=lua ts=3 sw=3


--
-- The Osc module generates a cosine wave at the given frequency. The output
-- range is -1.0 .. +1.0
--

function Dsp:Osc(init)

	local cos = math.cos
	local i, di = 0, 0

	return Dsp:Mod({
		id = "osc",
		description = "Sine oscillator",
		controls = {
			{
				id = "f",
				description = "Frequency",
				max = 20000,
				log = true,
				unit = "Hz",
				default = 440,
				fn_set = function(val)
					di = val * math.pi * 2 / srate 
				end
			},
		},
		fn_gen = function()
			i = i + di
			return cos(i)
		end
	}, init)
end

-- vi: ft=lua ts=3 sw=3


--
-- Stereo pan. Takes one or two inputs and pans them between the two outputs.
--

function Dsp:Pan(init)

	local v1, v2

	return Dsp:Mod({
		description = "Pan",
		controls = {
			{
				id = "pan",
				description = "Pan",
				min = -1,
				max = 1,
				default = 0,
				fn_set = function(val) 
					v1 = math.min(1 + val, 1)
					v2 = math.min(1 - val, 1)
				end,
			},
		},

		fn_gen = function(i1, i2)
			i2 = i2 or i1
			return i1*v1, i2*v2
		end

	}, init)

end

-- vi: ft=lua ts=3 sw=3


function Dsp:Pitchshift(init)

	local pr, prn, pw = 0, 0, 0
	local size = 0x10000
	local buf = {}
	local nmix = 50
	local mix = 0
	local dmax = 1200 
	local win = 250
	local step = 10
	local factor = 1
	
	local floor = math.floor

	for i = 0, size do buf[i] = 0 end

	local function wrap(i)
		return i % size
	end

	local function read(i) 
		return buf[floor(wrap(i))] 
	end
	
	local function read4(fi)
		local i = floor(fi)
		local f = fi - i
		local a, b, c, d = read(i-1), read(i), read(i+1), read(i+2)
		local c_b = c-b
		return b + f * ( c_b - 0.16667 * (1.-f) * ( (d - a - 3*c_b) * f + (d + 2*a - 3*b)))
	end

	local function find(pf, pt1, pt2)

		local cmax, ptmax = 0, pt1

		for pt = pt1, pt2-win, step do
			local c = 0
			for i = 0, win-1, step do
				c = c + read(pf+i) * read(pt+i)
			end
			if c > cmax then
				cmax = c
				ptmax = pt
			end
		end

		return ptmax
	end
	
	return Dsp:Mod({
		description = "Pitch shift",
		controls = {
			{
				id = "f",
				description = "Factor",
				default = 1,
				min = 0.5,
				max = 2,
				fmt = "%0.2f",
				fn_set = function(val) factor = val end,
			},
		},

		fn_gen = function(vi)

			if vi == "factor" then
				factor = arg
				return
			end

			buf[pw] = vi
			local vo = read4(pr)

			if mix > 0 then
				local f = mix / nmix
				vo = vo * f + read4(prn) * (1-f)
				mix = mix - 1
				if mix == 0 then pr = prn end
			end

			if mix == 0 then
				local d = (pw - pr) % size
				if factor < 1 then
					if d > dmax then
						mix = nmix
						prn = find(pr, pr+dmax/2, pr+dmax)
					end
				else
					if d < win or d > dmax * 2 then
						mix = nmix
						prn = find(pr, pr-dmax, pr-win)
					end
				end
			end

			pw = wrap(pw + 1)
			pr = wrap(pr + factor)
			prn = wrap(prn + factor)

			return vo
		end
	}, init)

end

-- vi: ft=lua ts=3 sw=3


-- 
-- Freeverb reverberator, based on Jezar's public domain C++ sources. 
--
-- This is a relatively simple and cheap stereo reverberator, based on a
-- cascade of comb and allpass filters.
--

function Dsp:Reverb(init)

	local function allpass(bufsize)
		local buffer = {}
		local feedback = 0
		local bufidx = 0
		return function(input)
			local bufout = buffer[bufidx] or 0
			local output = -input + bufout
			buffer[bufidx] = input + (bufout*feedback)
			bufidx = (bufidx + 1) % bufsize
			return output
		end
	end

	local comb_fb = 0
	local comb_damp1 = 0.5
	local comb_damp2 = 0.5

	local function fcomb(bufsize, feedback, damp)
		local buffer = {}
		local bufidx = 0
		local filterstore = 0
		return function(input)
			local output = buffer[bufidx] or 0
			local filterstore = (output*comb_damp2) + (filterstore*comb_damp1)
			buffer[bufidx] = input + (filterstore*comb_fb)
			bufidx = (bufidx + 1) % bufsize
			return output
		end
	end

	local fixedgain = 0.015
	local scalewet = 3
	local scaledry = 2
	local scaledamp = 0.4
	local scaleroom = 0.28
	local offsetroom = 0.7
	local stereospread = 23

	local comb, allp
	local gain
	local dry, wet1, wet2

	local comb, allp = { 
		{
			fcomb(1116), fcomb(1188), 
			fcomb(1277), fcomb(1356),
			fcomb(1422), fcomb(1491), 
			fcomb(1557), fcomb(1617),
		}, {
			fcomb(1116+stereospread), fcomb(1188+stereospread),
			fcomb(1277+stereospread), fcomb(1356+stereospread),
			fcomb(1422+stereospread), fcomb(1491+stereospread),
			fcomb(1557+stereospread), fcomb(1617+stereospread),
		}
	}, {
		{ 
			allpass(556), allpass(441), allpass(341), allpass(225), 
		}, { 
			allpass(556+stereospread), allpass(441+stereospread), 
			allpass(341+stereospread), allpass(225+stereospread), 
		}
	}

	local arg_wet, arg_dry, arg_room, arg_damp

	return Dsp:Mod({
		description = "Reverb",
		controls = {
			fn_update = function()
				local initialroom = arg_room 
				local initialdamp = arg_damp 
				local initialwet = arg_wet/scalewet
				local initialdry = arg_dry or 0
				local initialwidth = 2
				local initialmode = 0

				local wet = initialwet * scalewet
				local roomsize = (initialroom*scaleroom) + offsetroom
				dry = initialdry * scaledry
				local damp = initialdamp * scaledamp
				local width = initialwidth
				local mode = initialmode

				wet1 = wet*(width/2 + 0.5)
				wet2 = wet*((1-width)/2)

				comb_fb = roomsize
				comb_damp1 = damp
				comb_damp2 = 1 - damp
				gain = fixedgain

			end,
			{
				id = "wet",
				description = "Wet volume",
				default = 0.5,
				fmt = "%0.2f",
				fn_set = function(val) arg_wet = val end
			}, {
				id = "dry",
				description = "Dry volume",
				default = 0.5,
				fmt = "%0.2f",
				fn_set = function(val) arg_dry = val end
			}, {
				id = "room",
				description = "Room size",
				max = 1.1,
				default = 0.5,
				fmt = "%0.2f",
				fn_set = function(val) arg_room = val end
			}, {
				id = "damp",
				description = "Damping",
				default = 0.5,
				fmt = "%0.2f",
				fn_set = function(val) arg_damp = val end
			}
		},
		fn_gen = function(in1, in2)
			in2 = in2 or in1
			local input = (in1 + in2) * gain
			
			local out = { 0, 0 }

			for c = 1, 2 do
				for i = 1, #comb[c] do
					out[c] = out[c] + comb[c][i](input)
				end
				for i = 1, #allp[c] do
					out[c] = allp[c][i](out[c])
				end
			end

			local out1 = out[1]*wet1 + out[2]*wet2 + in1*dry
			local out2 = out[2]*wet1 + out[1]*wet2 + in2*dry

			return out1, out2
		end
	}, init)

end

-- vi: ft=lua ts=3 sw=3


--
-- The Saw module generates a sawtooth wave at the given frequency. The output
-- range is -1.0 .. +1.0
--

function Dsp:Saw(init)

	local v, dv = 0, 0

	return Dsp:Mod({
		description = "Saw tooth oscillator",
		controls = {
			{
				id = "f",
				description = "Frequency",
				max = 20000,
				log = true,
				unit = "Hz",
				default = 440,
				fn_set = function(val)
					dv = 2 * val / srate
				end,
			},
		},
		fn_gen = function()
			v = v + dv
			if v > 1 then v = v - 2 end
			return v
		end
	}, init)

end

-- vi: ft=lua ts=3 sw=3


--
-- The Square module generates a square wave at the given frequency and pwm
-- offset. The output range is -1.0 .. +1.0
--

function Dsp:Square(init)

	local saw = Dsp:Saw()
	local pwm = 0.5

	return Dsp:Mod({
		id = "osc",
		description = "Sine oscillator",
		controls = {
			{
				id = "f",
				description = "Frequency",
				max = 20000,
				log = true,
				unit = "Hz",
				default = 440,
				fn_set = function(val)
					saw:set { f = val }
				end
			}, {
				id = "pwm",
				description = "PWM",
				default = 0,
				fn_set = function(val)
					pwm = val
				end
			},
		},
		fn_gen = function()
			return saw() < pwm and -1 or 1
		end
	}, init)
end

-- vi: ft=lua ts=3 sw=3


function Dsp:Width(init)

	local a1, a2

	return Dsp:Mod({
		description = "Width",
		controls = {
			{
				id = "width",
				description = "Stereo width",
				min = 0,
				max = 2,
				default = 1,
				fn_set = function(v)
					a1 = math.min(0.5 + v/2, 1)
					a2 = 0.5 - v/2
				end,
			},
		},

		fn_gen = function(i1, i2)
			return i1 * a1 + i2 * a2,
			       i2 * a1 + i1 * a2
		end

	}, init)

end

-- vi: ft=lua ts=3 sw=3

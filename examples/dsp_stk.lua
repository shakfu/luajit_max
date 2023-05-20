-- dsp.lua

SAMPLE_RATE = 44100.0


function dump(o)
   if type(o) == 'table' then
      local s = '{ '
      for k,v in pairs(o) do
         if type(k) ~= 'number' then k = '"'..k..'"' end
         s = s .. '['..k..'] = ' .. dump(v) .. ','
      end
      return s .. '} '
   else
      return tostring(o)
   end
end



local _blitsquare = stk.BlitSquare(220.) -- BlitSquare(double frequency)
blitsquare = function(x, fb, n, frequency, phase, nHarmonics, p3)
   -- _blitsquare:reset()
   _blitsquare:setPhase(phase)
   -- _blitsquare:getPhase()
   _blitsquare:setFrequency(frequency)
   _blitsquare:setHarmonics(nHarmonics)
   -- _blitsquare:lastOut()
   return _blitsquare:tick()
end


local _pitshift = stk.PitShift() -- PitShift()
pitshift = function(x, fb, n, shift, p1, p2, p3)
   -- _pitshift:clear()
   _pitshift:setShift(shift)
   -- _pitshift:lastOut()
   return _pitshift:tick(x)
end


local _sine = stk.SineWave()
sine = function(x, fb, n, freq, time, phase, phase_offset)
   _sine:setFrequency(freq)
   _sine:addTime(time)
   _sine:addPhase(phase)
   _sine:addPhaseOffset(phase_offset)
   return _sine:tick()
end


local _delay = stk.Delay(9, 4096) -- (delay = 0, maxDelay = 4096)
delay = function(x, fb, n, delay, fb_ratio, balance, tap_delay)
   _delay:setDelay(delay)
   local x1 = x + (fb * fb_ratio)
   local wet_val = balance * _delay:tick(x1)
   local dry_val = (1 - balance) * x1
   return wet_val + dry_val
end


base = function(x, n, p0, p1, p2, p3)
   return x / 2
end


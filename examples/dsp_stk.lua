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


local _sine = stk.SineWave()
sine = function(x, fb, n, freq, time, phase, phase_offset)
    _sine:setFrequency(freq)
    _sine:addTime(time)
    _sine:addPhase(phase)
    _sine:addPhaseOffset(phase_offset)
    return _sine:tick()
end


-- local _delay = stk.Delay() -- (delay = 0, maxDelay = 4096)
-- delay = function(x, fb, n, delay)
--    _delay:setDelay(delay)

-- -- getMaximumDelay
-- -- setMaximumDelay
-- -- setDelay
-- -- getDelay
-- -- tapIn
-- -- tapOut
-- -- addTo
-- -- lastOut
-- -- nextOut
-- -- energy

-- end


base = function(x, n, p0, p1, p2, p4)
   return x / 2
end


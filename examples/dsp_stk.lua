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
sine = function(x, fb, n, freq)
    _sine:setFrequency(freq)
    -- _sine.addTime(time)
    -- _sine.addPhase(phase)
    -- _sine.addPhaseOffset(phase_offset)
    return _sine:tick()
end



base = function(x, n, p1)
   return x / 2
end


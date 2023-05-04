
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


-- -- low-pass single-pole filter
-- -- y(n) = y(n-1) + b(x(n) - y(n-1))
-- -- where 
-- --  b = 1 - d
-- --  d: Decay between samples (in (0, 1)).
-- -- see: https://tomroelandts.com/articles/low-pass-single-pole-iir-filter
lpf1 = function(x, x0, n, decay)
    local b = 1 - decay
    x0 = x0 + b * (x - x0)
    -- or just: x0 = x - decay*x - decay*x0
    return x0;
end

-- see: https://dsp.stackexchange.com/questions/60277/is-the-typical-implementation-of-low-pass-filter-in-c-code-actually-not-a-typica
lpf2 = function(x, x0, n, decay)
    x0 = (1 - decay) * x0 + decay * (x + x0)/2
    return x0;
end

-- see: https://www.musicdsp.org/en/latest/Filters/257-1-pole-lpf-for-smooth-parameter-changes.html
lpf3 = function(x, x0, n, alpha)
    local b = 1 - alpha;
    x0 = (x * b) + (x0 * alpha)
    return x0
end

-- a (0-1)
-- see: https://www.musicdsp.org/en/latest/Effects/42-soft-saturation.html
saturate = function(x, feedback, n, a)
   if x < 0 then
      return x
   elseif x > a then
      return a + (x-a)/(1+((x-a)/(1-a))^2)
   elseif x > 1 then
      return (a + 1)/2
   end
end
   


base = function(x, n, p1)
    return x / 2
end


dsp = lpf2

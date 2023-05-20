# Guide to Lua Files in this Directory

## Main module

Intended to be loaded by the respective external directly.

- `dsp.lua` : an example of a lua file which can be loaded by the `luajit~`
  external.

- `dsp_stk.lua`: an example of a lua file that can be loaded by the `luajit.stk~` external.

## Secondary support modules

These can be optionally imported by the main modules as required

- `dsp_worp.lua`: lua dsp algorithms extracted from the [worp](https://github.com/zevv/worp) dsp library.

- `fun.lua`: the functional helper library, [luafun](https://github.com/luafun/luafun),  which can be be imported and used in any of the main lua modules. Works well with luajit.

## Templates

- `dsp_stk_funcs.lua`: function templates which can be used in the stk
  external above.



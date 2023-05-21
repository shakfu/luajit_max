# Guide to Lua Files in this Directory

## luajit~

Intended to be loaded by the respective external directly.

- `dsp.lua` : an example of a lua file which can be loaded by the `luajit~`
  external.

The following module is imported

- `dsp_worp.lua`: lua dsp algorithms extracted from the [worp](https://github.com/zevv/worp) dsp library.


## luajit.stk~

Intended to be loaded by the respective external directly.

- `dsp_stk.lua`: an example of a lua file that can be loaded by the `luajit.stk~` external.

The following module provides a reference to the api of the converted stk classes in lua form. It's not supposed to be used directly. The idea is to copy the required function templates to the `dsp_stk.lua` file and implement them as required.

- `dsp_stk_funcs.lua`: function templates which can be used in the stk
  external above.


## Secondary support modules

- `fun.lua`: the functional helper library, [luafun](https://github.com/luafun/luafun),  which can be be imported and used in any of the main lua modules. Works well with luajit.



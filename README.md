# luajit_max

Includes two externals which both embed luajit in a Max/Msp external:

1. **luajit~**

A basic demo of embedding the luajit engine in a max/msp external.

A number of basic dsp functions are implemented in `examples/dsp.lua` and they can be selected by name from the dropdown in `help/luajit~.maxhelp`.

It is also possible to add or modify functions to `dsp.lua` and then send a `bang` message to reload it, provided the name of the function is either selected in the dropdown menu or is sent as a message to the `luajit~` object.


2. **luajit.stk~**

Same as (1) but with the addition of objects from the [Synthesis ToolKit (stk)](https://github.com/thestk/stk) library which are wrapped auotmatically using [LuaBridge3](https://github.com/kunitoki/LuaBridge3) and a custom python script.

In this the relevant lua file is called `dsp_stk.lua`. There is also another generated file, `dsp_funcs.lua`, which includes all of the stk wrapper lua functions with documentation.


## Installation

Just type the following:

```bash
git clone https://github.com/shakfu/luajit_max.git
make setup
make
```

Note: `make setup` does the following:

```bash
git submodule init
git submodule update
ln -s $(shell pwd) "$(HOME)/Documents/Max 8/Packages/$(shell basename `pwd`)"
```

## Usage

Open the help files for demonstrations of the externals.


## References

- [LuaJIT](https://luajit.org)
- [STK Introduction](http://www.music.mcgill.ca/~gary/307/week8/stk.html)
- [LuaBridge3](https://github.com/kunitoki/LuaBridge3)
- [cxxheaderparser](https://github.com/robotpy/cxxheaderparser) -- python c++ header parser used in the parser/generator script.


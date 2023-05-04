# luajit~

A basic demo of embedding the luajit engine in a max/msp external.

A number of basic dsp functions are implemented in `examples/dsp.lua` and they can be selected by name from the dropdown in `help/luajit~.maxhelp`.

It is also possible to add or modify functions to `dsp.lua` and then send a `bang` message to reload it, provided the name of the function is either selected in the dropdown menu or is sent as a message to the `luajit~` object.

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

Open `help/luajit~.maxhelp` and also `examples/dsp.lua` in your editor.
# luajit~

A basic demo of embedding the luajit engine in a max/msp external.

In this case, a lua function is called in the external's `perform64` method to makes it easy to 'live-code' the dsp function in lua by editing the `examples/script.lua` file and saving it.


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

Open `help/luajit~.maxhelp` and also `examples/script.lua` in your editor.
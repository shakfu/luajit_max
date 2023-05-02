SCRIPTS := source/scripts
BUILD := build
LUAJIT := $(BUILD)/deps/luajit-install/lib/libluajit-5.1.a

.PHONY: cmake fixup clean setup 

all: cmake


cmake: $(LUAJIT)
	@mkdir -p build && cd build && cmake .. && make

$(LUAJIT):
	@bash $(SCRIPTS)/build_dependencies.sh

fixup: cmake
	@bash $(SCRIPTS)/fix_bundle.sh


clean:
	@rm -rf build


setup:
	git submodule init
	git submodule update
	ln -s $(shell pwd) "$(HOME)/Documents/Max 8/Packages/$(shell basename `pwd`)"

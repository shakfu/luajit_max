
DEPS=../../../../build/deps
LUAJIT=${DEPS}/luajit-install
STK=${DEPS}/stk-install
LUABRIDGE=../includes/LuaBridge


g++ --std=c++17 \
	-I${LUABRIDGE} \
	-I${LUAJIT}/include/luajit-2.1 \
	-I${STK}/include/stk \
	-I. \
	-I${LUAJIT} \
	-framework CoreAudio \
	-framework CoreFoundation \
	-o main \
	main.cpp \
	${LUAJIT}/lib/libluajit-5.1.a \
	${STK}/lib/libstk.a


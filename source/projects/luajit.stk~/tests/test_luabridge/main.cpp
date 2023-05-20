// main.cpp
#include "SineWave.h"

#include "lua.hpp"
#include <LuaBridge.h>

#include <iostream>
#include <cstdlib>

namespace app {

class Person {
private:
    std::string name;
    int age;

public:
    Person(std::string name, int age) {
        this->name = name;
        this->age = age;
    }

    std::string identify() {
        return this->name + " " + std::to_string(this->age);
    }

    void load1( const char **filenames );

    void load( std::vector<std::string> filenames ) {
    }

};

} // end namespace app

using namespace luabridge;


void report_errors(lua_State *L, int status) {
    if (status == 0) {
        return;
    }

    std::cerr << "[LUA ERROR] " << lua_tostring(L, -1) << std::endl;

    // remove error message from Lua state
    lua_pop(L, 1);
}


int main() {
    lua_State* L = luaL_newstate();
    luaL_dofile(L, "cpp_call_lua.lua");
    luaL_openlibs(L);

    LuaRef s = getGlobal(L, "testString");
    LuaRef n = getGlobal(L, "number");

    // std::string luaString = s.cast<std::string>();
    std::string luaString = std::string(s);
    // int answer = n.cast<int>();
    int answer = (int) n;
    std::cout << luaString << std::endl;
    std::cout << "And here's our number:" << answer << std::endl;

    getGlobalNamespace(L)
        .beginNamespace("app")
            .beginClass <app::Person> ("Person")
                .addConstructor <void (*) (std::string name, int age)>()
                .addFunction("identify", &app::Person::identify)
                .addFunction("load", &app::Person::load)
            .endClass()
        .endNamespace();

    getGlobalNamespace(L)
        .beginNamespace("stk")
            .beginClass <stk::SineWave> ("SineWave")
                .addConstructor<void ()> ()
                .addFunction("reset", &stk::SineWave::reset)
                .addFunction("setRate", &stk::SineWave::setRate)
                .addFunction("setFrequency", &stk::SineWave::setFrequency)
                .addFunction("addTime", &stk::SineWave::addTime)
                .addFunction("addPhase", &stk::SineWave::addPhase)
                .addFunction("lastOut", &stk::SineWave::lastOut)
                .addFunction("tick", 
                    luabridge::overload<>(&stk::SineWave::tick),
                    luabridge::overload<stk::StkFrames&, unsigned int>(&stk::SineWave::tick))
            .endClass()
        .endNamespace();

    // load some code from Lua file
    int scriptload_status = luaL_dofile(L, "lua_call_cpp.lua");
    report_errors(L, scriptload_status);

    return 0;
}

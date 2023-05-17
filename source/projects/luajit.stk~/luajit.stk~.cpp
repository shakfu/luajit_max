/**
    @file
    luajit.stk~: luajit+stk for Max
*/

#include "SineWave.h"
#include "Delay.h"
#include "FreeVerb.h"
#include "Moog.h"

#include <cstdlib>

#include "ext.h"
#include "ext_obex.h"
#include "z_dsp.h"

#include "lua.hpp"
#include <LuaBridge.h>

#include <libgen.h>
#include <unistd.h>

enum {
    PARAM0 = 0, 
    PARAM1,
    PARAM2,
    PARAM3,
    MAX_INLET_INDEX // -> maximum number of inlets (0-based)
};

// struct to represent the object's state
typedef struct _lstk {
    t_pxobject ob;      // the object itself (t_pxobject in MSP instead of t_object)
    lua_State *L;       // lua state
    t_symbol* filename; // filename of lua file in Max search path
    t_symbol* funcname; // name of lua dsp function to use
    double param0;      // parameter 0 (leftmost)
    double param1;      // parameter 1
    double param2;      // parameter 2
    double param3;      // parameter 3 (rightmost)
    double v1;          // historical value;
    long m_in;          // space for the inlet number used by all of the proxies
    void *inlets[MAX_INLET_INDEX];
} t_lstk;


// method prototypes
void *lstk_new(t_symbol *s, long argc, t_atom *argv);
void lstk_init_lua(t_lstk *x);
void lstk_free(t_lstk *x);
void lstk_assist(t_lstk *x, void *b, long m, long a, char *s);
void lstk_bang(t_lstk *x);
void lstk_anything(t_lstk* x, t_symbol* s, long argc, t_atom* argv);
void lstk_float(t_lstk *x, double f);
void lstk_dsp64(t_lstk *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void lstk_perform64(t_lstk *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam);

t_string* get_path_from_package(t_class* c, char* subpath);

// global class pointer variable
static t_class *lstk_class = NULL;


//-----------------------------------------------------------------------------------------------


int run_lua_string(t_lstk *x, const char* code)
{
    int err;
    err = luaL_dostring(x->L, code);
    if (err) {
        error("%s", lua_tostring(x->L, -1));
        lua_pop(x->L, 1);  /* pop error message from the stack */
    }
    return 0; 
}

int run_lua_file(t_lstk *x, const char* path)
{
    int err;
    err = luaL_dofile(x->L, path);
    if (err) {
        error("%s", lua_tostring(x->L, -1));
        lua_pop(x->L, 1);  /* pop error message from the stack */
    }
    return 0; 
}

float lua_dsp(t_lstk *x, float audio_in, float audio_prev, float n_samples, 
                         float param0, float param1, float param2, float param3)
{
   lua_getglobal(x->L, x->funcname->s_name);
   // core
   lua_pushnumber(x->L, audio_in);
   lua_pushnumber(x->L, audio_prev);
   lua_pushnumber(x->L, n_samples);
   // params
   lua_pushnumber(x->L, param0);
   lua_pushnumber(x->L, param1);
   lua_pushnumber(x->L, param2);
   lua_pushnumber(x->L, param3);
   // Call the function with 7 arguments, returning 1 result
   lua_call(x->L, 7, 1);
   // Get the result
   float result = (float)lua_tonumber(x->L, -1);
   lua_pop(x->L, 1);
   return result;
}


t_string* get_path_from_external(t_class* c, char* subpath)
{
    char external_path[MAX_PATH_CHARS];
    char external_name[MAX_PATH_CHARS];
    char conform_path[MAX_PATH_CHARS];
    short path_id = class_getpath(c);
    t_string* result;

#ifdef __APPLE__
    const char* ext_filename = "%s.mxo";
#else
    const char* ext_filename = "%s.mxe64";
#endif
    snprintf_zero(external_name, MAX_PATH_CHARS, ext_filename, c->c_sym->s_name);
    path_toabsolutesystempath(path_id, external_name, external_path);
    path_nameconform(external_path, conform_path, PATH_STYLE_MAX, PATH_TYPE_BOOT);
    result = string_new(external_path);
    if (subpath != NULL) {
        string_append(result, subpath);
    }
    return result;
}


t_string* get_path_from_package(t_class* c, char* subpath)
{
    t_string* result;
    t_string* external_path = get_path_from_external(c, NULL);

    const char* ext_path_c = string_getptr(external_path);

    result = string_new(dirname(dirname((char*)ext_path_c)));

    if (subpath != NULL) {
        string_append(result, subpath);
    }

    return result;
}

//-----------------------------------------------------------------------------------------------

void ext_main(void *r)
{
    // object initialization, note the use of dsp_free for the freemethod, which is required
    // unless you need to free allocated memory, in which case you should call dsp_free from
    // your custom free function.

    t_class *c = class_new("luajit.stk~", (method)lstk_new, (method)lstk_free, (long)sizeof(t_lstk), 0L, A_GIMME, 0);

    class_addmethod(c, (method)lstk_float,    "float",    A_FLOAT, 0);
    class_addmethod(c, (method)lstk_anything, "anything", A_GIMME, 0);
    class_addmethod(c, (method)lstk_bang,     "bang",              0);
    class_addmethod(c, (method)lstk_dsp64,    "dsp64",    A_CANT,  0);
    class_addmethod(c, (method)lstk_assist,   "assist",   A_CANT,  0);

    class_dspinit(c);
    class_register(CLASS_BOX, c);
    lstk_class = c;
}


void lstk_run_file(t_lstk *x)
{
    if (x->filename != gensym("")) {
        char norm_path[MAX_PATH_CHARS];
        path_nameconform(x->filename->s_name, norm_path, 
            PATH_STYLE_MAX, PATH_TYPE_BOOT);
        if (access(norm_path, F_OK) == 0) { // file exists in path
            post("run %s", norm_path);
            run_lua_file(x, norm_path);
        } else { // try in the example folder
            t_string* path = get_path_from_package(lstk_class, "/examples/");
            string_append(path, x->filename->s_name);
            const char* lua_file = string_getptr(path);
            post("run %s", lua_file);
            run_lua_file(x, lua_file);
        }
    }    
}


void *lstk_new(t_symbol *s, long argc, t_atom *argv)
{
    t_lstk *x = (t_lstk *)object_alloc(lstk_class);

    if (x) {
        dsp_setup((t_pxobject *)x, 1);  // MSP inlets: arg is # of inlets and is REQUIRED!
        // use 0 if you don't need inlets

        outlet_new(x, "signal");        // signal outlet (note "signal" rather than NULL)
        x->param0 = 0.0;
        x->param1 = 0.0;
        x->param2 = 0.0;
        x->param3 = 0.0;
        x->v1 = 0.0;
        x->filename = atom_getsymarg(0, argc, argv); // 1st arg of object
        x->funcname = gensym("base");
        post("load: %s", x->filename->s_name);

        for(int i = (MAX_INLET_INDEX - 1); i > 0; i--) {
            x->inlets[i] = proxy_new((t_object *)x, i, &x->m_in);
        }

        // init lua
        lstk_init_lua(x);
    }
    return (x);
}


void lstk_free(t_lstk *x)
{
    lua_close(x->L);
    dsp_free((t_pxobject *)x);
    for(int i = (MAX_INLET_INDEX - 1); i > 0; i--) {
        object_free(x->inlets[i]);
    }
}


void lstk_assist(t_lstk *x, void *b, long m, long a, char *s)
{
    if (m == ASSIST_INLET) { //inlet
        sprintf(s, "I am inlet %ld", a);
    }
    else {  // outlet
        sprintf(s, "I am outlet %ld", a);
    }
}

void lstk_bang(t_lstk *x)
{
    lstk_run_file(x);
}

void lstk_anything(t_lstk* x, t_symbol* s, long argc, t_atom* argv)
{

    if (s != gensym("")) {
        post("funcname: %s", s->s_name);
        x->funcname = s;
    }
}


void lstk_float(t_lstk *x, double f)
{
    switch (proxy_getinlet((t_object *)x)) {
        case 0:
            post("received in inlet 0 (leftmost)");
            x->param0 = f;
            post("param0: %f", x->param0);
            break;
        case 1:
            post("received in inlet 1");
            x->param1 = f;
            post("param1: %f", x->param1);
            break;
        case 2:
            post("received in inlet 2");
            x->param2 = f;
            post("param2: %f", x->param2);
            break;
        case 3:
            post("received in inlet 3");
            x->param3 = f;
            post("param3: %f", x->param3);
            break;
    }
}


void lstk_dsp64(t_lstk *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    post("sample rate: %f", samplerate);
    post("maxvectorsize: %d", maxvectorsize);

    object_method(dsp64, gensym("dsp_add64"), x, lstk_perform64, 0, NULL);
}


void lstk_perform64(t_lstk *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    t_double *inL = ins[0];     // we get audio for each inlet of the object from the **ins argument
    t_double *outL = outs[0];   // we get audio for each outlet of the object from the **outs argument
    int n = sampleframes;       // n = 64
    double v1 = x->v1;

    while (n--) {
        v1 = lua_dsp(x, *inL++, v1, n, x->param0, x->param1, x->param2, x->param3);
        *outL++ = v1;
    }

    x->v1 = v1;

}


void lstk_init_lua(t_lstk *x)
{
    x->L = luaL_newstate();
    luaL_openlibs(x->L);  /* opens the standard libraries */

    luabridge::getGlobalNamespace(x->L)
        .beginNamespace("stk")
            .beginClass <stk::SineWave> ("SineWave")
                .addConstructor<void ()> ()
                .addFunction("reset", &stk::SineWave::reset)
                .addFunction("setRate", &stk::SineWave::setRate)
                .addFunction("setFrequency", &stk::SineWave::setFrequency)
                .addFunction("addTime", &stk::SineWave::addTime)
                .addFunction("addPhase", &stk::SineWave::addPhase)
                .addFunction("addPhaseOffset", &stk::SineWave::addPhaseOffset)
                .addFunction("lastOut", &stk::SineWave::lastOut)
                .addFunction("tick", 
                    luabridge::overload<>(&stk::SineWave::tick),
                    luabridge::overload<stk::StkFrames&, unsigned int>(&stk::SineWave::tick))
            .endClass()

            .beginClass <stk::Delay> ("Delay")
                .addConstructor<void (*) (unsigned long delay, unsigned long maxDelay)>()
                .addFunction("getMaximumDelay", &stk::Delay::getMaximumDelay)
                .addFunction("setMaximumDelay", &stk::Delay::setMaximumDelay)
                .addFunction("setDelay", &stk::Delay::setDelay)
                .addFunction("getDelay", &stk::Delay::getDelay)
                .addFunction("tapIn", &stk::Delay::tapIn)
                .addFunction("tapOut", &stk::Delay::tapOut)
                .addFunction("addTo", &stk::Delay::addTo)
                .addFunction("lastOut", &stk::Delay::lastOut)
                .addFunction("nextOut", &stk::Delay::nextOut)
                .addFunction("energy", &stk::Delay::energy)
                .addFunction("tick", 
                    luabridge::overload<stk::StkFloat>(&stk::Delay::tick),
                    luabridge::overload<stk::StkFrames&, unsigned int>(&stk::Delay::tick),
                    luabridge::overload<stk::StkFrames&, stk::StkFrames&, unsigned int, unsigned int>(&stk::Delay::tick))
            .endClass()

            .beginClass <stk::Moog> ("Moog")
                .addConstructor<void ()> ()
                .addFunction("noteOn", &stk::Moog::noteOn)
                .addFunction("setFrequency", &stk::Moog::setFrequency)
                .addFunction("setModulationSpeed", &stk::Moog::setModulationSpeed)
                .addFunction("setModulationSpeed", &stk::Moog::setModulationSpeed)
                .addFunction("controlChange", &stk::Moog::controlChange)
                .addFunction("tick", 
                    luabridge::overload<unsigned int>(&stk::Moog::tick),
                    luabridge::overload<stk::StkFrames&, unsigned int>(&stk::Moog::tick))
            .endClass()

            .beginClass <stk::FreeVerb> ("FreeVerb")
                .addConstructor<void ()> ()
                .addFunction("setEffectMix", &stk::FreeVerb::setEffectMix)
                .addFunction("setRoomSize", &stk::FreeVerb::setRoomSize)
                .addFunction("getRoomSize", &stk::FreeVerb::getRoomSize)
                .addFunction("setDamping", &stk::FreeVerb::setDamping)
                .addFunction("getDamping", &stk::FreeVerb::getDamping)
                .addFunction("setWidth", &stk::FreeVerb::setWidth)
                .addFunction("getWidth", &stk::FreeVerb::getWidth)
                .addFunction("setMode", &stk::FreeVerb::setMode)
                .addFunction("getMode", &stk::FreeVerb::getMode)
                .addFunction("clear", &stk::FreeVerb::clear)
                .addFunction("lastOut", &stk::FreeVerb::lastOut)
                .addFunction("tick", 
                    luabridge::overload<stk::StkFloat, stk::StkFloat, unsigned int>(&stk::FreeVerb::tick),
                    luabridge::overload<stk::StkFrames&, unsigned int>(&stk::FreeVerb::tick),
                    luabridge::overload<stk::StkFrames&, stk::StkFrames&, unsigned int, unsigned int>(&stk::FreeVerb::tick))
            .endClass()
        .endNamespace();

    lstk_run_file(x);
}


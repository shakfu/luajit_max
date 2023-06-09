/**
    @file
    luajit~: luajit for Max
*/

#include "ext.h"
#include "ext_obex.h"
#include "z_dsp.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <libgen.h>
#include <unistd.h>

#define USE_LUA 1


// struct to represent the object's state
typedef struct _mlj {
    t_pxobject ob;      // the object itself (t_pxobject in MSP instead of t_object)
    lua_State *L;       // lua state
    t_symbol* filename; // filename of lua file in Max search path
    t_symbol* funcname; // name of lua dsp function to use
    double param1;      // the value of a property of our object
    double v1;          // historical value;
} t_mlj;


// method prototypes
void *mlj_new(t_symbol *s, long argc, t_atom *argv);
void mlj_init_lua(t_mlj *x);
void mlj_free(t_mlj *x);
void mlj_assist(t_mlj *x, void *b, long m, long a, char *s);
void mlj_bang(t_mlj *x);
void mlj_anything(t_mlj* x, t_symbol* s, long argc, t_atom* argv);
void mlj_float(t_mlj *x, double f);
void mlj_dsp64(t_mlj *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags);
void mlj_perform64(t_mlj *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam);

t_string* get_path_from_package(t_class* c, char* subpath);

// global class pointer variable
static t_class *mlj_class = NULL;


//-----------------------------------------------------------------------------------------------


int run_lua_string(t_mlj *x, const char* code)
{
    int err;
    err = luaL_dostring(x->L, code);
    if (err) {
        error("%s", lua_tostring(x->L, -1));
        lua_pop(x->L, 1);  /* pop error message from the stack */
    }
    return 0; 
}

int run_lua_file(t_mlj *x, const char* path)
{
    int err;
    err = luaL_dofile(x->L, path);
    if (err) {
        error("%s", lua_tostring(x->L, -1));
        lua_pop(x->L, 1);  /* pop error message from the stack */
    }
    return 0; 
}

float lua_dsp(t_mlj *x, float audio_in, float audio_prev, float n_samples, float param1) {
   lua_getglobal(x->L, x->funcname->s_name);
   lua_pushnumber(x->L, audio_in);
   lua_pushnumber(x->L, audio_prev);
   lua_pushnumber(x->L, n_samples);
   lua_pushnumber(x->L, param1);
   // Call the function with 4 arguments, returning 1 result
   lua_call(x->L, 4, 1);
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

    t_class *c = class_new("luajit~", (method)mlj_new, (method)mlj_free, (long)sizeof(t_mlj), 0L, A_GIMME, 0);

    class_addmethod(c, (method)mlj_float,    "float",    A_FLOAT, 0);
    class_addmethod(c, (method)mlj_anything, "anything", A_GIMME, 0);
    class_addmethod(c, (method)mlj_bang,     "bang",              0);
    class_addmethod(c, (method)mlj_dsp64,    "dsp64",    A_CANT,  0);
    class_addmethod(c, (method)mlj_assist,   "assist",   A_CANT,  0);

    class_dspinit(c);
    class_register(CLASS_BOX, c);
    mlj_class = c;
}


void mlj_run_file(t_mlj *x)
{
    if (x->filename != gensym("")) {
        char norm_path[MAX_PATH_CHARS];
        path_nameconform(x->filename->s_name, norm_path, 
            PATH_STYLE_MAX, PATH_TYPE_BOOT);
        if (access(norm_path, F_OK) == 0) { // file exists in path
            post("run %s", norm_path);
            run_lua_file(x, norm_path);
        } else { // try in the example folder
            t_string* path = get_path_from_package(mlj_class, "/examples/");
            string_append(path, x->filename->s_name);
            const char* lua_file = string_getptr(path);
            post("run %s", lua_file);
            run_lua_file(x, lua_file);
        }
    }    
}


void mlj_init_lua(t_mlj *x)
{
    x->L = luaL_newstate();
    luaL_openlibs(x->L);  /* opens the standard libraries */
    mlj_run_file(x);
}


void *mlj_new(t_symbol *s, long argc, t_atom *argv)
{
    t_mlj *x = (t_mlj *)object_alloc(mlj_class);

    if (x) {
        dsp_setup((t_pxobject *)x, 1);  // MSP inlets: arg is # of inlets and is REQUIRED!
        // use 0 if you don't need inlets

        outlet_new(x, "signal");        // signal outlet (note "signal" rather than NULL)
        x->param1 = 0.0;
        x->v1 = 0.0;
        x->filename = atom_getsymarg(0, argc, argv); // 1st arg of object
        x->funcname = gensym("base");
        post("filename: %s", x->filename->s_name);

        // init lua
        mlj_init_lua(x);
    }
    return (x);
}


void mlj_free(t_mlj *x)
{
    lua_close(x->L);
    dsp_free((t_pxobject *)x);
}


void mlj_assist(t_mlj *x, void *b, long m, long a, char *s)
{
    if (m == ASSIST_INLET) { //inlet
        sprintf(s, "I am inlet %ld", a);
    }
    else {  // outlet
        sprintf(s, "I am outlet %ld", a);
    }
}

void mlj_bang(t_mlj *x)
{
    mlj_run_file(x);
}

void mlj_anything(t_mlj* x, t_symbol* s, long argc, t_atom* argv)
{

    if (s != gensym("")) {
        post("funcname: %s", s->s_name);
        x->funcname = s;
    }
}


void mlj_float(t_mlj *x, double f)
{
    x->param1 = f;
    post("param1: %f", x->param1);
}


void mlj_dsp64(t_mlj *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    post("sample rate: %f", samplerate);
    post("maxvectorsize: %d", maxvectorsize);

    object_method(dsp64, gensym("dsp_add64"), x, mlj_perform64, 0, NULL);
}


#if defined USE_LUA

void mlj_perform64(t_mlj *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    t_double *inL = ins[0];     // we get audio for each inlet of the object from the **ins argument
    t_double *outL = outs[0];   // we get audio for each outlet of the object from the **outs argument
    int n = sampleframes;       // n = 64
    double v1 = x->v1;

    while (n--) {
        v1 = lua_dsp(x, *inL++, v1, n, x->param1);
        *outL++ = v1;
    }

    x->v1 = v1;

}

#else

// experiment in c here

void mlj_perform64(t_mlj *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    t_double *inL = ins[0];     // we get audio for each inlet of the object from the **ins argument
    t_double *outL = outs[0];   // we get audio for each outlet of the object from the **outs argument
    int n = sampleframes;       // n = 64
    double v0;
    double b = 1 - x->param1;
    double v1 = x->v1;

    while (n--) {
        v0 = (*inL++);
        v1 += b * (v0 - v1);
        *outL++ = v1;
    }
    x->v1 = v1;
}

#endif

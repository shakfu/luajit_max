/**
    @file
    luajit~: luajit for Max
    original by: jeremy bernstein, jeremy@bootsquad.com
    @ingroup examples
*/

#include "ext.h"            // standard Max include, always required (except in Jitter)
#include "ext_obex.h"       // required for "new" style objects
#include "z_dsp.h"          // required for MSP objects

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <libgen.h>
#include <unistd.h>


// struct to represent the object's state
typedef struct _mlj {
    t_pxobject ob;         // the object itself (t_pxobject in MSP instead of t_object)
    double     offset;     // the value of a property of our object
    lua_State *L;          // lua state
    t_symbol*  filename;   // filename of lua file in Max search path
} t_mlj;


// method prototypes
void *mlj_new(t_symbol *s, long argc, t_atom *argv);
void *mlj_init_lua(t_mlj *x);
void mlj_free(t_mlj *x);
void mlj_assist(t_mlj *x, void *b, long m, long a, char *s);
void mlj_bang(t_mlj *x);
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


int luaAdd(t_mlj *x, int a, int b) {
   // Push the add function on the top of the lua stack
   lua_getglobal(x->L, "add");
   // Push the first argument on the top of the lua stack
   lua_pushnumber(x->L, a);
   // Push the second argument on the top of the lua stack
   lua_pushnumber(x->L, b);
   // Call the function with 2 arguments, returning 1 result
   lua_call(x->L, 2, 1);
   // Get the result
   int sum = (int)lua_tointeger(x->L, -1);
   lua_pop(x->L, 1);
   return sum;
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
    class_addmethod(c, (method)mlj_bang,     "bang",              0);
    class_addmethod(c, (method)mlj_dsp64,    "dsp64",    A_CANT,  0);
    class_addmethod(c, (method)mlj_assist,   "assist",   A_CANT,  0);

    class_dspinit(c);
    class_register(CLASS_BOX, c);
    mlj_class = c;
}


void *mlj_init_lua(t_mlj *x)
{
    x->L = luaL_newstate();
    luaL_openlibs(x->L);  /* opens the standard libraries */
    if (x->filename != gensym("")) {
        char norm_path[MAX_PATH_CHARS];
        path_nameconform(x->filename->s_name, norm_path, 
            PATH_STYLE_MAX, PATH_TYPE_BOOT);
        if (access(norm_path, F_OK) == 0) { // file exists in path
            run_lua_file(x, norm_path);
        } else { // try in the example folder
            t_string* path = get_path_from_package(mlj_class, "/examples/");
            string_append(path, x->filename->s_name);
            const char* lua_file = string_getptr(path);
            run_lua_file(x, lua_file);
        }
    }
}

void *mlj_new(t_symbol *s, long argc, t_atom *argv)
{
    t_mlj *x = (t_mlj *)object_alloc(mlj_class);

    if (x) {
        dsp_setup((t_pxobject *)x, 1);  // MSP inlets: arg is # of inlets and is REQUIRED!
        // use 0 if you don't need inlets

        outlet_new(x, "signal");        // signal outlet (note "signal" rather than NULL)
        x->offset = 0.0;
        x->filename = atom_getsymarg(0, argc, argv); // 1st arg of object
        post("filename: %s", x->filename->s_name);

        // init lua
        mlj_init_lua(x);
    }
    return (x);
}




// NOT CALLED!, we use dsp_free for a generic free function
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
    post("lua add(10,2): %d", luaAdd(x, 10,2));
}


void mlj_float(t_mlj *x, double f)
{
    x->offset = f;
    post("offset: %f", x->offset);
}



// registers a function for the signal chain in Max
void mlj_dsp64(t_mlj *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    post("my sample rate is: %f", samplerate);

    // instead of calling dsp_add(), we send the "dsp_add64" message to the object representing the dsp chain
    // the arguments passed are:
    // 1: the dsp64 object passed-in by the calling function
    // 2: the symbol of the "dsp_add64" message we are sending
    // 3: a pointer to your object
    // 4: a pointer to your 64-bit perform method
    // 5: flags to alter how the signal chain handles your object -- just pass 0
    // 6: a generic pointer that you can use to pass any additional data to your perform method

    object_method(dsp64, gensym("dsp_add64"), x, mlj_perform64, 0, NULL);
}


// this is the 64-bit perform method audio vectors
void mlj_perform64(t_mlj *x, t_object *dsp64, double **ins, long numins, double **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    t_double *inL = ins[0];     // we get audio for each inlet of the object from the **ins argument
    t_double *outL = outs[0];   // we get audio for each outlet of the object from the **outs argument
    int n = sampleframes;

    // this perform method simply copies the input to the output, offsetting the value
    while (n--)
        *outL++ = *inL++ + x->offset;
}


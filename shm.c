#include "m_pd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#include <tchar.h>
#else
#include <sys/shm.h>
#endif

static t_class * shm_class;

typedef struct _shm
{
    t_object x_obj;
    t_outlet *x_outlet_0;
    
    int isServer;
    int isReady;
    int memorySize;
    unsigned char *sharedData;
    
    char memoryKey[MAXPDSTRING];
    
} t_shm;

static char *version = "$vertion: 0.1 $";
static void shm_key(t_shm *x, t_symbol *s);
static void shm_size(t_shm *x, t_float f);

//---------------------------------------------------------- new

static void *shm_new(t_symbol *s, int argc, t_atom *argv)
{
    t_shm *x = (t_shm *)pd_new(shm_class);
    x->x_outlet_0 = outlet_new(&x->x_obj, gensym("float"));
    
    if (argc > 0)
    {
        //----------
        // load arguments
        //----------
        while (argc--) {
            switch (argv->a_type) {
                case A_FLOAT: shm_size(x, atom_getfloat(argv)); break;
                case A_SYMBOL: shm_key(x, atom_getsymbol(argv)); break;
                default: logpost(x, 1, "[shm] err: Unsipported atom type"); break;
            }
            argv++;
        }
    }
    
    return (x);
}

//---------------------------------------------------------- delete

static void shm_free(t_shm *x)
{
}

//---------------------------------------------------------- bang
static void shm_bang(t_shm *x)
{
    logpost(x, 2, "[shm] notice: memory key = %s", x->memoryKey);
    logpost(x, 2, "[shm] notice: memory size = %d", x->memorySize);
}


static void shm_key(t_shm *x, t_symbol *s)
{
    strcpy(x->memoryKey, s->s_name);
    logpost(x, 2, "[shm] notice: set new memory key = %s", x->memoryKey);
}

static void shm_size(t_shm *x, t_float f)
{
    x->memorySize = round(f);
    logpost(x, 2, "[shm] notice: set new memory size = %d", x->memorySize);
}


//---------------------------------------------------------- setup

void shm_setup(void)
{
    shm_class = class_new(gensym("shm"),
                               (t_newmethod)shm_new,
                               (t_method)shm_free,
                               sizeof(t_shm),
                               CLASS_DEFAULT, A_GIMME, 0);
    
    class_addbang(shm_class, shm_bang);
    class_addmethod(shm_class, (t_method)shm_key, gensym("key"), A_DEFSYMBOL, 0);
    class_addmethod(shm_class, (t_method)shm_size, gensym("size"), A_FLOAT, 0);
    
    logpost(NULL, 2, "[shm] %s, author: Tatsuya Ogusu", version);
    logpost(NULL, 2, "[shm] is still in development, tcompiled against Pd version %d.%d.%d",PD_MAJOR_VERSION, PD_MINOR_VERSION, PD_BUGFIX_VERSION);
}



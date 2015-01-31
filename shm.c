#include "m_pd.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/shm.h>

static t_class * shm_class;

static const char   DEF_MEMORY_KEY[MAXPDSTRING] = "pd_shm";
static const int    DEF_MEMORY_SIZE = 256;

typedef struct _shm
{
    t_object x_obj;
    t_outlet *x_outlet; ///< dump shared data
    
    int             isReady;
    int             isServer;
    unsigned int    memorySize;
    char            memoryKey[MAXPDSTRING];
    char            dataType[MAXPDSTRING];
    void            * sharedData;
    
} t_shm;

static char *shm_version = "$vertion: 0.0.1 $";

static void shm_key(t_shm *x, t_symbol *s);     /// set memory key
static void shm_size(t_shm *x, t_float f);      /// set memory size
static void shm_get(t_shm *x, t_symbol *s, int argc, t_atom *argv);    /// get share data as type
static void shm_connect(t_shm *x);              /// try connect to shared memory
static void shm_disconnect(t_shm *x);           /// disconnect
static void shm_print(t_shm *x);                /// dump infomation

//---------------------------------------------------------- new

static void *shm_new(t_symbol *s, int argc, t_atom *argv)
{
    int i;
    
    t_shm *x = (t_shm *)pd_new(shm_class);
//    outlet_new(&x->x_obj, &s_list);
    x->x_outlet = outlet_new(&x->x_obj, &s_bang);
    
    // init
    x->memorySize = 0;
    x->isReady = 0;
    x->isServer = 0;
    x->sharedData = NULL;
    for (i = 0; i < MAXPDSTRING; ++i)
    {
        x->memoryKey[i] = 0;
        x->dataType[i] = 0;
    }
    
    if (argc > 0)
    {
        // get arguments
        while (argc--)
        {
            switch (argv->a_type)
            {
                case A_FLOAT: shm_size(x, atom_getfloat(argv)); break;
                case A_SYMBOL: shm_key(x, atom_getsymbol(argv)); break;
                default: logpost(x, 1, "[shm] err: Unsupported atom type"); break;
            }
            argv++;
        }
        if (x->memorySize > 0)
        {
            shm_connect(x);
        }
    }
    else {
        // set default
        strcpy(x->memoryKey, DEF_MEMORY_KEY);
        x->memorySize = DEF_MEMORY_SIZE;
    }
    
    return (x);
}

//---------------------------------------------------------- delete

static void shm_free(t_shm *x)
{
    if (x->isReady != 0) shm_disconnect(x);
}

//---------------------------------------------------------- inlet action
static void shm_bang(t_shm *x)
{
    if (x->isReady != 0 && x->sharedData != NULL)
    {
//        post("%f", (float *)x->sharedData);
        outlet_float(x->x_outlet, *(float *)x->sharedData);
    }
//    post("bang %d", x->isReady);
}

static void shm_key(t_shm *x, t_symbol *s)
{
    strcpy(x->memoryKey, s->s_name);
    logpost(x, 2, "[shm] notice: set new memory key = %s", x->memoryKey);
}

static void shm_size(t_shm *x, t_float f)
{
    x->memorySize = floor(f);
    logpost(x, 2, "[shm] notice: set new memory size = %d", x->memorySize);
}

static void shm_get(t_shm *x, t_symbol *s, int argc, t_atom *argv)
{
    if (argc > 0)
    {
        while (argc--)
        {
            switch (argv->a_type)
            {
                case A_FLOAT: shm_size(x, atom_getfloat(argv)); break;
                case A_SYMBOL: shm_key(x, atom_getsymbol(argv)); break;
                default: logpost(x, 1, "[shm] err: Unsupported atom type"); break;
            }
            argv++;
        }
    }
}

static void shm_print(t_shm *x)
{
    logpost(x, 2, "[shm] notice: memory key = %s", x->memoryKey);
    logpost(x, 2, "[shm] notice: memory size = %d", x->memorySize);
}

static void shm_connect(t_shm *x)
{
    int i;
    int sum;
    int fd;
    
    x->isReady = 0;
    
    // check key and size
    if (x->memorySize == 0)
    {
        logpost(x, 1, "[shm] err: still not set memory size");
        return;
    }
    sum = 0;
    for (i = 0; i < MAXPDSTRING; ++i) sum += (int)x->memoryKey[i];
    if (sum == 0)
    {
        logpost(x, 1, "[shm] err: still not set memory key");
        return;
    }
    
    
    // create/connect to shared memory from dummy file
    if ((fd = shm_open(x->memoryKey,  O_CREAT | O_RDWR, S_IRUSR | S_IWUSR)) != -1)
    {
        // map to memory
        ftruncate(fd, x->memorySize);
        x->sharedData = mmap(0, x->memorySize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (x->sharedData != NULL) {
            x->isReady = 1;
            logpost(x, 2, "[shm] notice: connection succeed key: %s, size: %d", x->memoryKey, x->memorySize);
        }
        else {
            logpost(x, 1, "[shm] err: failed connect to shared memory");
            if (x->isServer) shm_unlink(x->memoryKey);
        }
    }
    else {
        logpost(x, 1, "[shm] err: failed open to shared memory");
    }
}

static void shm_disconnect(t_shm *x)
{
    if (x->isServer)
    {
        munmap(x->sharedData, x->memorySize);
        shm_unlink(x->memoryKey);
        x->isReady = 0;
        logpost(x, 2, "[shm] notice: dissconnect");
    }
}

static void shm_any(t_shm *x, t_symbol *s, int argc, t_atom *argv)
{
    if (s != NULL) post("%c", s);
    if (x->isReady != 0)
    {
        while(argc--)
        {
            post("%f", atom_getfloat(argv));
            argv++;
        }
    }
    else logpost(x, 1, "[shm] err: still not connected");
}

//static void shm_list(t_shm *x, t_symbol *s, int argc, t_atom *argv)
//{
//    shm_any(x, 0, argc, argv);
//}



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
    class_addmethod(shm_class, (t_method)shm_get, gensym("get"), A_GIMME, 0);
    class_addmethod(shm_class, (t_method)shm_print, gensym("print"), 0);
    class_addmethod(shm_class, (t_method)shm_connect, gensym("connect"), A_GIMME, 0);
    class_addmethod(shm_class, (t_method)shm_disconnect, gensym("disconnect"), 0);
//    class_addlist(shm_class, shm_list);
//    class_addanything(shm_class, shm_any);
    
    logpost(NULL, 2, "[shm] %s, author: Tatsuya Ogusu", shm_version);
    logpost(NULL, 2, "[shm] is still in development, tcompiled against Pd version %d.%d.%d",PD_MAJOR_VERSION, PD_MINOR_VERSION, PD_BUGFIX_VERSION);
}



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

typedef struct _shm
{
    t_object        x_obj;
    t_outlet*       x_outlet;
    t_atom          x_outv;

    int             isReady;
    int             isServer;
    unsigned int    memorySize;
    char            memoryKey[MAXPDSTRING];
    void*           sharedData;
    
} t_shm;

static char *shm_version = "$vertion: 0.0.1 $";

static void shm_key(t_shm *x, t_symbol *s);     /// set memory key
static void shm_size(t_shm *x, t_float f);      /// set memory size
static void shm_get(t_shm *x, t_symbol *s, int argc, t_atom *argv); /// get memory mapped file
static void shm_set(t_shm *x, t_symbol *s, int argc, t_atom *argv); /// set memory mapped file
static void shm_connect(t_shm *x, t_symbol *s, int argc, t_atom *argv); /// try connect to shared memory
static void shm_disconnect(t_shm *x);           /// disconnect
static void shm_print(t_shm *x);                /// dump infomation

//---------------------------------------------------------- new

static void *shm_new(t_symbol *s, int argc, t_atom *argv)
{
    int i;
    
    t_shm *x = (t_shm *)pd_new(shm_class);
    outlet_new(&x->x_obj, &s_list);
    x->x_outlet = outlet_new(&x->x_obj, &s_bang);
    
    // init
    x->memorySize = 0;
    x->isReady = 0;
    x->isServer = 0;
    x->sharedData = NULL;
    
    if (argc > 0)
    {
        shm_connect(x, s, argc, argv);
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
        outlet_float(x->x_outlet, *(float *)x->sharedData);
    }
    else logpost(x, 1, "[shm] err: still not connected");
}

static void shm_key(t_shm *x, t_symbol *s)
{
    strcpy(x->memoryKey, s->s_name);
//    logpost(x, 2, "[shm] notice: set new memory key = %s", x->memoryKey);
}

static void shm_size(t_shm *x, t_float f)
{
    x->memorySize = floor(f);
//    logpost(x, 2, "[shm] notice: set new memory size = %d", x->memorySize);
}


static void shm_get(t_shm *x, t_symbol *s, int argc, t_atom *argv)
{
    if (argc == 0)
    { 
        logpost(x, 1, "[shm] err: empty argument");
        return;
    }
    if (x->isReady == 0 || x->sharedData == NULL)
    {
        logpost(x, 1, "[shm] err: still not connected");
        return;
    }
    
    void* data = x->sharedData;
    unsigned int type = 0;
    unsigned int index = 0;
    int i;
    
    for (i = 0; i < argc; ++i)
    {
        if (argv->a_type == A_FLOAT)
        {
            index = atom_getint(argv);
            if (index >= x->memorySize)
            {
                logpost(x, 1, "[shm] err: ");
                index = 0;
            }
        }
        if (argv->a_type == A_SYMBOL)
        {
            char c = *(atom_getsymbol(argv)->s_name);
            if (c == 'i') type = 1;
            if (c == 'f') type = 2;
            if (c == 's') type = 3;
        }
        argv++;
    }

    if (type == 0)
    {
        logpost(x, 1, "[shm] err: unknown type");
        return;
    }

    if (type == 1)
    {
        data += sizeof(int)*index;
        outlet_float(x->x_obj.ob_outlet, *(float *)data);
    }
    if (type == 2)
    {
        data += sizeof(float)*index;
        outlet_float(x->x_obj.ob_outlet, *(float *)data);
    }
    if (type == 3)
    {
        data += sizeof(char)*index;
        outlet_symbol(x->x_obj.ob_outlet, *(char *)data);
    }
}

static void shm_set(t_shm *x, t_symbol *s, int argc, t_atom *argv)
{
    if (argc == 0)
    { 
        logpost(x, 1, "[shm] err: empty argument");
        return;
    }
    if (x->isReady == 0 || x->sharedData == NULL)
    {
        logpost(x, 1, "[shm] err: still not connected");
        return;
    }
    
    void* data = x->sharedData;
    unsigned int type = 0;
    unsigned int index = 0;
    float got_float;
    int i;

    if (argv->a_type == A_SYMBOL)
    {
        char c = *(atom_getsymbol(argv)->s_name);
        if (c == 'i') type = 1;
        if (c == 'f') type = 2;
        if (c == 's') type = 3;
    }
    else {
        logpost(x, 1, "[shm] err: arg1 unknown type");
        return;
    }
    argv++;

    if (argv->a_type == A_FLOAT)
    {
        index = atom_getint(argv);
    }
    else {
        logpost(x, 1, "[shm] err: arg1 unknown type");
        return;
    } 
    argv++;

    if (argv->a_type == A_FLOAT)
    {
        got_float = atom_getfloat(argv);
    }

    if (type == 0)
    {
        logpost(x, 1, "[shm] err: unknown type");
        return;
    }

    if (type == 1)
    {
        data += sizeof(int)*index;
    }
    if (type == 2)
    {
        data += sizeof(float)*index;
        *(float *)data = got_float;
    }
    if (type == 3)
    {
        data += sizeof(char)*index;
    }
}

static void shm_print(t_shm *x)
{
    logpost(x, 2, "[shm] notice: memory key = %s", x->memoryKey);
    logpost(x, 2, "[shm] notice: memory size = %d", x->memorySize);
}

static void shm_connect(t_shm *x, t_symbol *s, int argc, t_atom *argv)
{
    int i;
    int sum;
    int fd;
    
    x->isReady = 0;
    
    // set key and size
    if (argc > 0)
    {
        for (i = 0; i < argc; ++i)
        {
            if (i == 0)
            {
                if (argv->a_type == A_SYMBOL)
                {
                    shm_key(x, atom_getsymbol(argv));
                }
                else logpost(x, 1, "[shm] err: arg 1 unsupported atom type");
            }
            if (i == 1)
            {
                if (argv->a_type == A_FLOAT)
                {
                    shm_size(x, atom_getint(argv));
                }
                else logpost(x, 1, "[shm] err: arg 2 unsupported atom type");
            }
            if (i == 2)
            {
                if (argv->a_type == A_FLOAT)
                {
                    x->isServer = (atom_getfloat(argv) != 0);
                }
                else logpost(x, 1, "[shm] err: arg 3 unsupported atom type");
            }
            argv++;
        }
    }
    else {
        logpost(x, 1, "[shm] err: empty argument");
        return;
    }
    
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
            logpost(x, 2, "[shm] notice: connection succeed [key %s][size %d][server %d]", x->memoryKey, x->memorySize, x->isServer);
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


//---------------------------------------------------------- setup

void shm_setup(void)
{
    shm_class = class_new(gensym("shm"),
                               (t_newmethod)shm_new,
                               (t_method)shm_free,
                               sizeof(t_shm),
                               CLASS_DEFAULT, A_GIMME, 0);
    
    class_addbang(shm_class, shm_bang);
//    class_addmethod(shm_class, (t_method)shm_key, gensym("key"), A_DEFSYMBOL, 0);
//    class_addmethod(shm_class, (t_method)shm_size, gensym("size"), A_FLOAT, 0);
    class_addmethod(shm_class, (t_method)shm_get, gensym("get"), A_GIMME, 0);
    class_addmethod(shm_class, (t_method)shm_set, gensym("set"), A_GIMME, 0);
    class_addmethod(shm_class, (t_method)shm_print, gensym("print"), 0);
    class_addmethod(shm_class, (t_method)shm_connect, gensym("connect"), A_GIMME, 0);
    class_addmethod(shm_class, (t_method)shm_disconnect, gensym("disconnect"), 0);
    
    logpost(NULL, 2, "[shm] %s, author: Tatsuya Ogusu", shm_version);
    logpost(NULL, 2, "[shm] is still in development, tcompiled against Pd version %d.%d.%d",PD_MAJOR_VERSION, PD_MINOR_VERSION, PD_BUGFIX_VERSION);
}



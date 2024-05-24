#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <utils/server.h>
#include <pthread.h>
#include <semaphore.h>
#include "conexiones.h"
#include "estructuras.h"
#include "memoriaCPU.h"
#include "memoriaKernel.h"
#include "memoriaDeInstrucciones.h"

extern int socket_kernel;
extern int socket_cpu;
extern int socket_io;
void* escuchar_kernel();
void* escuchar_cpu();

typedef struct{
    int socket_kernel;
    int socket_cpu;
    int tiempo_retardo;
}t_parametros_cpu;

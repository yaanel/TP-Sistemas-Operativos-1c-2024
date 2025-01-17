#ifndef RECS_H
#define RECS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <utils/pcb.h>
#include <planificadorLP.h>

extern t_config* config;

typedef struct {
    char* nombre;
    int instancias;
    t_queue* procesosBloqueados;
} t_recurso;

// Lee los recursos del archivo de configuracion e instancia para cada uno
// un t_recurso
void leerRecursosDeConfig();

// Retorna el recurso que matchee el nombre pasado por parametro
t_recurso* hallarRecurso(char* nombre);

// Asigna un recurso a un proceso. Retorna 1 en caso de que el recurso este disponible
// y 0 si el proceso tuvo que ser bloqueado 
int waitRecurso(t_recurso* recurso, PCB* proceso);

// Suma 1 a la cantidad de instancias de un recurso y, en caso de haber
// un proceso bloqueado, envia ese proceso a Ready
void signalRecurso(t_recurso* recurso, PCB* proceso);

// Libera todos los recursos utilizados por un proceso
void liberarRecursos(PCB* proceso);

void freeRec(t_recurso* recurso);

#endif /* RECS_H */
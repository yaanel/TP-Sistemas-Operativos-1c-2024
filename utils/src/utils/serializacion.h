#ifndef SER_H
#define SER_H
#include "pcb.h"
#include<stdio.h>
#include<stdlib.h>
#include <string.h>
#include<netdb.h>
#include <unistd.h>

typedef enum {
    PAQUETE,
    // Kernel - IO
    CONEXION_IOGENERICA,
    CONEXION_STDIN,
    CONEXION_STDOUT,

    // Kernel - CPU
    ENVIO_PCB,
    INTERRUPCION,

    //Kernel - Memoria
    CREACION_PROCESO, //Acá Kernel me envia path
    FIN_PROCESO,
    //CPU o IO - Memoria
    ACCESO_ESPACIO_USUARIO,
    //Memoria de Instrucciones - CPU
    ENVIO_DE_INSTRUCCIONES,
    // Cualquier módulo a Memoria
    AJUSTAR_SIZE_PROCESO,
    ACCESO_TABLAS_PAGINAS,

}op_code;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;


t_paquete* crear_paquete(void);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
t_buffer* recibir_buffer(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
int recibir_operacion(int socket_cliente);

void agregar_int_a_paquete(t_paquete* paquete, int valor);
void agregar_string_a_paquete(t_paquete* paquete, char* string);
void agregar_PCB_a_paquete(t_paquete* paquete, PCB* pcb);
void agregar_instruccion_a_paquete(t_paquete* paquete, t_instruccion* instruccion);

int buffer_read_int(t_buffer* buffer);
uint32_t buffer_read_uint32(t_buffer* buffer);
char* buffer_read_string(t_buffer* buffer);
PCB* buffer_read_pcb(t_buffer* buffer);
t_instruccion* buffer_read_instruccion(t_buffer* buffer);


#endif /* SER_H */
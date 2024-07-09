#include "interfaces.h"

void esperarClientesIO(t_conexion_escucha* params) {
    while(1) {
        int* socket_cliente = malloc(sizeof(int));
        pthread_t hilo;

        *socket_cliente = esperar_cliente(params->socket_servidor, params->modulo);

        if (*socket_cliente < 0) {
            free(socket_cliente);
            break;
        }

        void (*func)(int*);

        switch(recibir_operacion(*socket_cliente)) {
            case CONEXION_IOGENERICA:
                func = &administrarInterfazGenerica;
                break;
            case CONEXION_STDIN:
                func = &administrarSTDIN;
                break;
            case CONEXION_STDOUT:
                func = &administrarSTDOUT;
                break;
            case CONEXION_DIAL_FS:
                func = &administrarDIALFS;
            default:
                log_error(logger, "Conexión inválida de una interfaz");
                break;
        }
        
        pthread_create(&hilo,
                        NULL,
                        (void*) func,
                        socket_cliente);
        pthread_detach(hilo);
    }
}

t_IOConectada* IOConectado_create(int socket_cliente, tipoInterfaz tipo) {
    t_IOConectada* interfaz = malloc(sizeof(t_IOConectada));

    t_buffer* buffer = recibir_buffer(socket_cliente);

    interfaz->nombreInterfaz = buffer_read_string(buffer);
    interfaz->tipo = tipo;
    interfaz->procesosBloqueados = queue_create();
    pthread_mutex_init(&(interfaz->mutex), NULL);
    sem_init(&(interfaz->semaforo), 0, 0);

    pthread_mutex_lock(&mutexListaInterfaces);
    list_add(interfacesConectadas, interfaz);
    pthread_mutex_unlock(&mutexListaInterfaces);

    return interfaz;
}

void manejarSTDINOUT(int* socket_cliente, t_IOConectada* interfaz) {
    op_code op;

    while (1) {
        t_paquete* paquete = crear_paquete(PAQUETE); // despues lo cambio por uno que tenga mas sentido
        sem_wait(&interfaz->semaforo);

        pthread_mutex_lock(&interfaz->mutex);
        t_solicitudIOSTDIN_OUT* solicitud = queue_pop(interfaz->procesosBloqueados);
        pthread_mutex_unlock(&interfaz->mutex);

        agregar_int_a_paquete(paquete, solicitud->proceso->PID);
        while(list_size(solicitud->direcciones) > 0 ) {
            t_direccionMemoria* dir = list_remove(solicitud->direcciones, 0);
            agregar_uint32_a_paquete(paquete, dir->direccion);
            agregar_uint32_a_paquete(paquete, dir->tamanio);
            free(dir);
        }

        enviar_paquete(paquete, *socket_cliente);
        eliminar_paquete(paquete);

        op = recibir_operacion(*socket_cliente);
        if (op <= 0) {
            log_error(logger, "La operación de IO STDIN/STDOUT no se pudo completar exitosamente");
            enviarAExit(solicitud->proceso, INVALID_INTERFACE);
            free(solicitud);
            break;
        }
        
        pthread_mutex_lock(&mutexPlanificador);
        planificar(op, solicitud->proceso, NULL);
        pthread_mutex_unlock(&mutexPlanificador);

        list_destroy(solicitud->direcciones);
        free(solicitud);
    } 
}

void administrarInterfazGenerica(int* socket_cliente) {

    t_IOConectada* interfaz = IOConectado_create(*socket_cliente, INTERFAZ_GENERICA);

    op_code op;

    while (1) {
        t_paquete* paquete = crear_paquete(PAQUETE); // despues lo cambio por uno que tenga mas sentido
        sem_wait(&interfaz->semaforo);

        pthread_mutex_lock(&interfaz->mutex);
        t_solicitudIOGenerica* solicitud = queue_pop(interfaz->procesosBloqueados);
        pthread_mutex_unlock(&interfaz->mutex);

        agregar_int_a_paquete(paquete, solicitud->unidadesTrabajo);
        enviar_paquete(paquete, *socket_cliente);
        eliminar_paquete(paquete);

        op = recibir_operacion(*socket_cliente);
        if (op <= 0) {
            log_error(logger, "La operación de IO genérica no se pudo completar exitosamente");
            enviarAExit(solicitud->proceso, INVALID_INTERFACE);
            free(solicitud);
            break;
        }
        
        pthread_mutex_lock(&mutexPlanificador);
        planificar(op, solicitud->proceso, NULL);
        pthread_mutex_unlock(&mutexPlanificador);

        free(solicitud);
    } 
}

void administrarSTDIN(int* socket_cliente) {
    t_IOConectada* interfaz = IOConectado_create(*socket_cliente, INTERFAZ_STDIN);

    manejarSTDINOUT(socket_cliente, interfaz);
}

void administrarSTDOUT(int* socket_cliente) {
    t_IOConectada* interfaz = IOConectado_create(*socket_cliente, INTERFAZ_STDOUT);

    manejarSTDINOUT(socket_cliente, interfaz);
}

void administrarDIALFS(int* socket_cliente) {
    t_IOConectada* interfaz = IOConectado_create(*socket_cliente, INTERFAZ_DIALFS);

    while (1) {
        t_paquete* paquete = crear_paquete(PAQUETE); // es posible que nunca lo cambie por algo que tenga mas sentido
        sem_wait(&interfaz->semaforo);

        pthread_mutex_lock(&interfaz->mutex);
        t_solicitudDIALFS* solicitud = queue_pop(interfaz->procesosBloqueados);
        pthread_mutex_unlock(&interfaz->mutex);

        switch (solicitud->operacion) {
            case iIO_FS_CREATE:
                // algo
                break;
            case iIO_FS_DELETE:
                // otro algo
                break;
            case iIO_FS_TRUNCATE:
                // mas algoses
                break;
            case iIO_FS_READ:
                // o sea digamos algo
                break;
            case iIO_FS_WRITE:
                // pongamoslo en estos terminos: algo
                break;
            default:
                log_error(logger, "Operación invalida"); // esto no deberia ser posible pero si no gcc se pone pesado con los casos no usados
                break;
        }

        op_code op = recibir_operacion(*socket_cliente);
        if (op <= 0) {
            log_error(logger, "La operación de File System no se pudo completar exitosamente");
            enviarAExit(solicitud->proceso, INVALID_INTERFACE);
            free(solicitud);
            break;
        }
        
        pthread_mutex_lock(&mutexPlanificador);
        planificar(op, solicitud->proceso, NULL);
        pthread_mutex_unlock(&mutexPlanificador);

        free(solicitud);
    } 

}

t_solicitudIOGenerica* solicitudIOGenerica_create(PCB* proceso, t_buffer* buffer) {
    t_solicitudIOGenerica* solicitud = malloc(sizeof(t_solicitudIOGenerica));
    solicitud->proceso = proceso;
    solicitud->unidadesTrabajo = buffer_read_int(buffer);

    return solicitud;
}

t_solicitudIOSTDIN_OUT* solicitudIOSTDIN_OUT_create(PCB* proceso, t_buffer* buffer) {
    t_solicitudIOSTDIN_OUT* solicitud = malloc(sizeof(t_solicitudIOSTDIN_OUT));
    solicitud->proceso = proceso;
    t_list* direcciones = list_create();
    
    while(buffer->size > 0) {
        t_direccionMemoria* dir = malloc(sizeof(t_direccionMemoria));
        dir->direccion = buffer_read_uint32(buffer);
        dir->tamanio = buffer_read_uint32(buffer);

        list_add(direcciones, dir);
    }

    solicitud->direcciones = direcciones;

    return solicitud;
}

t_solicitudDIALFS* solicitudDIALFS_create(PCB* proceso, op_code operacion, t_buffer* buffer) {
    t_solicitudDIALFS* solicitud = malloc(sizeof(t_solicitudDIALFS));
    solicitud->proceso = proceso;
    solicitud->operacion = operacion;

    switch (operacion) {
        case iIO_FS_CREATE:
        case iIO_FS_DELETE:
            solicitud->nombreArchivo = buffer_read_string(buffer);
            break;
        case iIO_FS_TRUNCATE:
            solicitud->nombreArchivo = buffer_read_string(buffer);
            solicitud->tamanio = buffer_read_int(buffer);
            break;
        case iIO_FS_READ:
        case iIO_FS_WRITE:
            solicitud->nombreArchivo = buffer_read_string(buffer);
            solicitud->direccion = buffer_read_int(buffer);
            solicitud->tamanio  = buffer_read_int(buffer);
            solicitud->ubicacionPuntero = buffer_read_int(buffer);
            break;
        default:
            break;
    }

    return solicitud;
}

bool comprobarOperacionValida(t_IOConectada* interfaz, op_code operacion) {

    bool opValida;

    switch (interfaz->tipo) {
        case INTERFAZ_GENERICA:
            opValida = operacion == ENVIAR_IO_GEN_SLEEP;
            break;
        case INTERFAZ_STDIN:
            opValida = operacion == ENVIAR_IO_STDIN_READ;
            break;
        case INTERFAZ_STDOUT:
            opValida = operacion == ENVIAR_IO_STDOUT_WRITE;
            break;
        // Falta todo lo de FS
        default:
            opValida = false;
            break;
    }

    return opValida;
}

t_IOConectada* hallarInterfazConectada(char* nombre) {

    bool _mismoNombre(t_IOConectada* interfaz) {
        return !strcmp(nombre, interfaz->nombreInterfaz);
    };

    pthread_mutex_lock(&mutexListaInterfaces);
    t_IOConectada* interfaz = list_find(interfacesConectadas, (void *) _mismoNombre);
    pthread_mutex_unlock(&mutexListaInterfaces);

    return interfaz;
}
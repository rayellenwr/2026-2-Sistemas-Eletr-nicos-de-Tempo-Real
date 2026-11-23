#ifndef MAQUINA_H
#define MAQUINA_H

#include <mqueue.h>
#include <pthread.h>
#include <semaphore.h>

#define BANDEJA 6           /* posicoes da bandeja de componentes (buffer) */
#define POS_PLACA 12        /* componentes por placa */
#define ALIMENTADORES 2
#define CABECOTES 2
#define FILA_NOME "/insersora_eventos"

typedef enum { RESISTOR, CAPACITOR } tipo_t;
typedef enum { EV_ALIMENTOU, EV_INSERIU, EV_PLACA_OK, EV_FIM } ev_tipo_t;

/* mensagem enviada pela fila POSIX ao processo monitor */
typedef struct {
    ev_tipo_t tipo;
    long id;        /* id da thread que gerou o evento */
    tipo_t comp;    /* componente envolvido */
    int pos;        /* posicao na bandeja ou na placa */
} evento_t;

static inline const char *nome_comp(tipo_t t) {
    return t == RESISTOR ? "resistor" : "capacitor";
}

/* estado compartilhado (definido em main.c) */
extern tipo_t bandeja[BANDEJA];
extern int in, out;
extern int placa[POS_PLACA];
extern int inseridos;
extern sem_t vagas, prontas;
extern pthread_mutex_t m_bandeja, m_eixo, m_placa;
extern pthread_cond_t placa_completa;
extern mqd_t fila;

/* threads (threads.c) */
void *alimentador(void *arg);
void *cabecote(void *arg);
void *supervisor(void *arg);
void enviar_evento(ev_tipo_t tipo, long id, tipo_t comp, int pos);

/* processo monitor (monitor.c) */
void monitor_loop(mqd_t q);

#endif

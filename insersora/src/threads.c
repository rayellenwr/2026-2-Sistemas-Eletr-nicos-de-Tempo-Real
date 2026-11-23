/* Threads da maquina: alimentadores, cabecotes e supervisor. */
#include <stdlib.h>
#include <unistd.h>
#include "maquina.h"

void enviar_evento(ev_tipo_t tipo, long id, tipo_t comp, int pos) {
    evento_t ev = { tipo, id, comp, pos };
    mq_send(fila, (const char *)&ev, sizeof ev, 0);   /* IPC: fila de mensagens */
}

void *alimentador(void *arg) {
    long id = (long)arg;
    tipo_t t = (id == 1) ? RESISTOR : CAPACITOR;
    for (int i = 0; i < POS_PLACA / ALIMENTADORES; i++) {
        usleep(100000 + rand() % 200000);          /* trocando fita/reel */
        sem_wait(&vagas);                          /* espera posicao livre */
        pthread_mutex_lock(&m_bandeja);
        int pos = in;
        bandeja[in] = t;
        in = (in + 1) % BANDEJA;
        pthread_mutex_unlock(&m_bandeja);
        sem_post(&prontas);                        /* avisa: ha componente */
        enviar_evento(EV_ALIMENTOU, id, t, pos);
    }
    return NULL;
}

void *cabecote(void *arg) {
    long id = (long)arg;
    for (int i = 0; i < POS_PLACA / CABECOTES; i++) {
        sem_wait(&prontas);                        /* espera componente */
        pthread_mutex_lock(&m_bandeja);
        tipo_t t = bandeja[out];
        out = (out + 1) % BANDEJA;
        pthread_mutex_unlock(&m_bandeja);
        sem_post(&vagas);                          /* libera posicao */

        pthread_mutex_lock(&m_eixo);               /* eixo XY exclusivo */
        usleep(150000);                            /* mover + inserir */
        pthread_mutex_lock(&m_placa);              /* ordem fixa: eixo -> placa */
        int pos = inseridos;
        placa[pos] = t + 1;
        enviar_evento(EV_INSERIU, id, t, pos);     /* antes do signal: ordem correta no log */
        inseridos++;
        if (inseridos == POS_PLACA)
            pthread_cond_signal(&placa_completa);  /* acorda o supervisor */
        pthread_mutex_unlock(&m_placa);
        pthread_mutex_unlock(&m_eixo);
    }
    return NULL;
}

void *supervisor(void *arg) {
    (void)arg;
    pthread_mutex_lock(&m_placa);
    while (inseridos < POS_PLACA)
        pthread_cond_wait(&placa_completa, &m_placa);  /* dorme ate a placa fechar */
    pthread_mutex_unlock(&m_placa);
    enviar_evento(EV_PLACA_OK, 0, RESISTOR, POS_PLACA);
    return NULL;
}

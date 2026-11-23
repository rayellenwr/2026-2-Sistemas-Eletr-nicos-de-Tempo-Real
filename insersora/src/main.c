/* Maquina inseridora de componentes (pick-and-place) concorrente.
 * Processo principal: cria a fila POSIX, faz fork do monitor e dispara as threads. */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include "maquina.h"

tipo_t bandeja[BANDEJA];
int in = 0, out = 0;
int placa[POS_PLACA];
int inseridos = 0;

sem_t vagas;                 /* posicoes livres na bandeja (inicia em BANDEJA) */
sem_t prontas;               /* componentes disponiveis    (inicia em 0) */
pthread_mutex_t m_bandeja = PTHREAD_MUTEX_INITIALIZER;  /* protege in/out/bandeja */
pthread_mutex_t m_eixo    = PTHREAD_MUTEX_INITIALIZER;  /* eixo XY: um cabecote por vez */
pthread_mutex_t m_placa   = PTHREAD_MUTEX_INITIALIZER;  /* protege placa/inseridos */
pthread_cond_t  placa_completa = PTHREAD_COND_INITIALIZER;
mqd_t fila;

int main(void) {
    struct mq_attr attr = {0};
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(evento_t);

    mq_unlink(FILA_NOME);    /* limpa resto de execucao anterior */
    fila = mq_open(FILA_NOME, O_CREAT | O_RDWR, 0600, &attr);
    if (fila == (mqd_t)-1) { perror("mq_open"); return 1; }

    pid_t pid = fork();      /* fork ANTES de criar threads */
    if (pid < 0) { perror("fork"); mq_unlink(FILA_NOME); return 1; }
    if (pid == 0) {          /* processo filho = monitor */
        monitor_loop(fila);
        mq_close(fila);
        _exit(0);
    }

    sem_init(&vagas, 0, BANDEJA);
    sem_init(&prontas, 0, 0);

    pthread_t a[ALIMENTADORES], c[CABECOTES], s;
    pthread_create(&s, NULL, supervisor, NULL);
    for (long i = 0; i < ALIMENTADORES; i++) pthread_create(&a[i], NULL, alimentador, (void *)(i + 1));
    for (long i = 0; i < CABECOTES; i++)     pthread_create(&c[i], NULL, cabecote, (void *)(i + 1));

    for (int i = 0; i < ALIMENTADORES; i++) pthread_join(a[i], NULL);
    for (int i = 0; i < CABECOTES; i++)     pthread_join(c[i], NULL);
    pthread_join(s, NULL);

    enviar_evento(EV_FIM, 0, RESISTOR, 0);   /* manda o monitor encerrar */
    int status;
    waitpid(pid, &status, 0);                /* espera o processo filho */

    sem_destroy(&vagas);
    sem_destroy(&prontas);
    mq_close(fila);
    mq_unlink(FILA_NOME);
    return 0;
}

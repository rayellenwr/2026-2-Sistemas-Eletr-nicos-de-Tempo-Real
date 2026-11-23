/* Processo monitor: recebe eventos da maquina pela fila de mensagens POSIX. */
#include <stdio.h>
#include <unistd.h>
#include "maquina.h"

void monitor_loop(mqd_t q) {
    evento_t ev;
    int alimentados = 0, inseridos_total = 0;

    printf("[Monitor pid=%d] aguardando eventos em %s\n", getpid(), FILA_NOME);
    fflush(stdout);
    for (;;) {
        ssize_t n = mq_receive(q, (char *)&ev, sizeof ev, NULL);   /* bloqueia */
        if (n < 0) { perror("mq_receive"); return; }
        switch (ev.tipo) {
        case EV_ALIMENTOU:
            alimentados++;
            printf("[Alimentador %ld] %-9s -> bandeja[%d]\n", ev.id, nome_comp(ev.comp), ev.pos);
            break;
        case EV_INSERIU:
            inseridos_total++;
            printf("    [Cabecote %ld] inseriu %-9s na posicao %d\n", ev.id, nome_comp(ev.comp), ev.pos);
            break;
        case EV_PLACA_OK:
            printf("[Supervisor] placa concluida com %d componentes\n", ev.pos);
            break;
        case EV_FIM:
            printf("[Monitor] resumo: %d alimentados, %d inseridos. Encerrando.\n",
                   alimentados, inseridos_total);
            fflush(stdout);
            return;
        }
        fflush(stdout);
    }
}

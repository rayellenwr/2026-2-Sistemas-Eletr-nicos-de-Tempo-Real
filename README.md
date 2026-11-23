# Máquina Insersora de Componentes Concorrente
 
Simulação em C de uma máquina **pick-and-place** (inserção de componentes eletrônicos em placas SMT),
usada para demonstrar **programação concorrente** e **comunicação/sincronização entre processos e threads** em POSIX.
 
> Atividade: Avaliação Parcial 2 – Projeto Final Livre (Programação Concorrente e IPC)
> Equipe: Rayellen da Silva Araujo
 
## 1. Objetivo do projeto
 
Em uma máquina insersora, **alimentadores** fornecem componentes (resistores, capacitores) e **cabeçotes**
os inserem na placa. Dois recursos são críticos:
 
- a **bandeja** de componentes, um buffer limitado compartilhado (problema **produtor–consumidor**);
- o **eixo XY**, que só pode ser usado por um cabeçote por vez (**exclusão mútua**; dois cabeçotes juntos causariam colisão).
O projeto simula esse cenário com 5 threads e um processo monitor, mostrando como semáforos, mutexes, variável de
condição e fila de mensagens resolvem o problema sem condição de corrida e sem deadlock.
 
## 2. Como executar
 
**Requisitos:** Linux, `gcc` e `make` (testado com gcc 13 / Ubuntu 24). A biblioteca de filas POSIX (`-lrt`) já vem com a glibc.
 
```bash
git clone https://github.com/rayellenwr/2026-2-Sistemas-Eletr-nicos-de-Tempo-Real.git
cd 2026-2-Sistemas-Eletr-nicos-de-Tempo-Real/insersora
make          # compila
./insersora   # executa (ou: make run)
make clean    # remove binário e objetos
```
 
Uma execução completa produz **uma placa com 12 componentes** e termina sozinha (cerca de 2 s).
 
### Exemplo de saída
 
```
[Monitor pid=126] aguardando eventos em /insersora_eventos
[Alimentador 1] resistor  -> bandeja[0]
[Alimentador 2] capacitor -> bandeja[1]
    [Cabecote 1] inseriu resistor  na posicao 0
...
    [Cabecote 1] inseriu resistor  na posicao 10
    [Cabecote 2] inseriu capacitor na posicao 11
[Supervisor] placa concluida com 12 componentes
[Monitor] resumo: 12 alimentados, 12 inseridos. Encerrando.
```
 
## 3. Estrutura do código
 
O projeto fica na pasta `insersora/` deste repositório:
 
```
insersora/
├── include/maquina.h   # constantes, tipos, estado compartilhado e protótipos
├── src/main.c          # cria a fila POSIX, faz fork do monitor, dispara e junta as threads
├── src/threads.c       # alimentador, cabeçote e supervisor
├── src/monitor.c       # processo monitor: lê eventos da fila de mensagens
├── Makefile
└── README.md
```
 
## 4. Arquitetura
 
```
 Alimentador 1 ─┐                         ┌─ Cabeçote 1 ─┐
                ├─► Bandeja (buffer 6) ──►┤              ├─► Placa (12 pos.) ─► Supervisor
 Alimentador 2 ─┘   vagas / prontas       └─ Cabeçote 2 ─┘     eixo XY (mutex)   (cond. var.)
 
  todas as threads ──── mq_send ────► [fila POSIX] ────► processo Monitor (fork)
```
 
## 5. Mecanismos de IPC e sincronização utilizados
 
| Mecanismo | Onde | Função |
|---|---|---|
| `fork` / `waitpid` | `main.c` | cria o processo monitor e espera seu término |
| `pthread_create` / `pthread_join` | `main.c` | 5 threads: 2 alimentadores, 2 cabeçotes, 1 supervisor |
| **Fila de mensagens POSIX** (`mq_open`, `mq_send`, `mq_receive`, `mq_unlink`) | `main.c`, `threads.c`, `monitor.c` | **IPC**: as threads enviam eventos ao processo monitor |
| **Semáforos POSIX** `vagas` e `prontas` (`sem_init`, `sem_wait`, `sem_post`) | alimentador, cabeçote | controlam a **capacidade** da bandeja (produtor–consumidor) |
| **Mutex** `m_bandeja` | alimentador, cabeçote | exclusão mútua nos índices `in`/`out` e no vetor da bandeja |
| **Mutex** `m_eixo` | cabeçote | um cabeçote por vez no eixo XY (seção crítica) |
| **Mutex** `m_placa` | cabeçote, supervisor | protege a placa e o contador `inseridos` |
| **Variável de condição** `placa_completa` (`pthread_cond_wait` / `signal`) | cabeçote, supervisor | o supervisor dorme, sem gastar CPU, até a placa ficar completa |
 
## 6. Decisões de projeto
 
- **Semáforo × mutex:** os semáforos controlam *quantos* itens/vagas existem; o mutex controla *quem* mexe nos dados.
- **Sem deadlock:** o `sem_wait` vem **antes** de qualquer `lock` (nunca se espera num semáforo segurando um mutex), e os
  locks aninhados têm **ordem fixa** (`m_eixo` → `m_placa`).
- **`fork` antes das threads:** o monitor é criado antes de qualquer thread, evitando os problemas de usar `fork` em programa multithread.
- **Término limpo:** o processo principal envia `EV_FIM` pela fila, espera o monitor com `waitpid` e remove a fila com `mq_unlink`.
- **Evento antes do `signal`:** o evento de inserção é enviado antes de acordar o supervisor, garantindo a ordem lógica do log.
## 7. Validação
 
- Compilação sem avisos com `-Wall -Wextra`.
- 5 execuções consecutivas: sempre 12 componentes inseridos, sem travamentos.
- Compilado com `-fsanitize=thread` (ThreadSanitizer): **nenhum aviso de corrida de dados**.
Para repetir o teste do ThreadSanitizer:
 
```bash
gcc -g -fsanitize=thread -pthread -Iinclude src/*.c -o insersora_tsan -lrt && ./insersora_tsan
```
 
## 8. Possíveis extensões
 
- Mais de um processo (alimentação e visão em programas separados) usando **memória compartilhada** (`shm_open` + `mmap`).
- `SIGINT` como **parada de emergência** da máquina.
- Modo "sem sincronização" para demonstrar a condição de corrida.
 

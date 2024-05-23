#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <pthread.h>
#include <time.h>

#define LARGURA 22
#define ALTURA 12
#define CORPO_COBRINHA '*'
#define COMIDA '@'
#define PAREDE '#'
#define CIMA 'w'
#define BAIXO 's'
#define ESQUERDA 'a'
#define DIREITA 'd'
#define DELAY_HORIZONTAL 200000 // Atraso para movimentos horizontais
#define DELAY_VERTICAL 300000   // Atraso para movimentos verticais

// Definição da estrutura do nó da lista
typedef struct Node {
    int x;
    int y;
    struct Node* prox;
} Node;

// Definição da estrutura da cobra
typedef struct {
    Node* cabeca; // Aponta para a cabeça da cobra
    Node* cauda; // Aponta para a cauda da cobra
} Cobrinha;

// Definição da estrutura para o relógio
typedef struct {
    int minutos;
    int segundos;
} Relogio;

// Função para criar um novo nó
Node* criarNode(int x, int y) {
    Node* novoNode = (Node*)malloc(sizeof(Node));
    if (novoNode == NULL) {
        printf("Erro: Não foi possível alocar memória para um novo nó.\n");
        exit(EXIT_FAILURE);
    }
    novoNode->x = x;
    novoNode->y = y;
    novoNode->prox = NULL;
    return novoNode;
}

// Função para adicionar um novo nó no final da lista
void append(Cobrinha* cobrinha, int x, int y) {
    Node* novoNode = criarNode(x, y);
    if (cobrinha->cabeca == NULL) {
        cobrinha->cabeca = novoNode;
        cobrinha->cauda = novoNode;
    } else {
        cobrinha->cauda->prox = novoNode;
        cobrinha->cauda = novoNode;
    }
}

// Função para imprimir os elementos da lista
void printLista(Cobrinha* cobrinha) {
    Node* atual = cobrinha->cabeca;
    while (atual != NULL) {
        printf("(%d,%d) ", atual->x, atual->y);
        atual = atual->prox;
    }
    printf("\n");
}

// Função para liberar a memória alocada para a lista
void freeLista(Cobrinha* cobrinha) {
    if (cobrinha->cabeca != NULL) {
        Node* atual = cobrinha->cabeca;
        Node* prox;
        while (atual != NULL) {
            prox = atual->prox;
            free(atual);
            atual = prox;
        }
        cobrinha->cabeca = NULL;
        cobrinha->cauda = NULL;
    }
}

void configurarTerminalPadrao() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag |= ICANON;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void configurarTerminal() {
    struct termios t;
    tcgetattr(0, &t);
    t.c_lflag &= ~ICANON;
    tcsetattr(0, TCSANOW, &t);
}

int kbhit(void) {
    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv) == 1;
}

void printTela(char tela[ALTURA][LARGURA]) {
    system("clear");
    for(int i = 0; i < ALTURA; i++) {
        for(int j = 0; j < LARGURA; j++) {
            printf("%c", tela[i][j]);
        }
        printf("\n");
    }
}

void inicializarCobrinha(Cobrinha* cobrinha) {
    append(cobrinha, LARGURA / 2, ALTURA / 2);
    append(cobrinha, LARGURA / 2 - 1, ALTURA / 2);
    append(cobrinha, LARGURA / 2 - 2, ALTURA / 2);
}

void* atualizarRelogio(void* arg) {
    Relogio* relogio = (Relogio*)arg;
    while (1) {
        sleep(1);
        relogio->segundos++;
        if (relogio->segundos == 60) {
            relogio->segundos = 0;
            relogio->minutos++;
        }
    }
    return NULL;
}

int main() {
    char tela[ALTURA][LARGURA];
    char direcao;
    int comidaX = 0, comidaY = 0;
    int pontos = 0;

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    Cobrinha cobrinha = {NULL, NULL}; // Inicializa a cobra com a cabeça e a cauda como NULL
    inicializarCobrinha(&cobrinha);
    direcao = DIREITA;

    int pid = fork(); // Cria um novo processo

    if (pid < 0) {
        printf("Erro ao criar processo filho.\n");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // Processo filho
        close(pipefd[0]);  // Fecha a extremidade de leitura do pipe no processo filho

        configurarTerminal();

        while(1) {
            if(kbhit()) {
                direcao = getchar();
                write(pipefd[1], &direcao, sizeof(char)); // Escreve a direção no pipe
            }
            usleep(DELAY_HORIZONTAL); // Pequeno atraso para evitar loop infinito
        }
    } else { // Processo pai
        close(pipefd[1]);  // Fecha a extremidade de escrita do pipe no processo pai

        pthread_t thread_relogio;
        Relogio relogio = {0, 0};

        if (pthread_create(&thread_relogio, NULL, atualizarRelogio, (void*)&relogio) != 0) {
            printf("Erro ao criar thread do relógio.\n");
            exit(EXIT_FAILURE);
        }

        while(1) {
            char direcao_filho;
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(pipefd[0], &fds);
            int maxfd = pipefd[0] + 1;
            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 10000; // 10ms timeout
            if (select(maxfd, &fds, NULL, NULL, &timeout) > 0) { // Verifica se há dados disponíveis para leitura
                if (read(pipefd[0], &direcao_filho, sizeof(char)) != -1) {
                    direcao = direcao_filho;
                }
            }

            // Inicializa a tela
            for(int i = 0; i < ALTURA; i++) {
                for(int j = 0; j < LARGURA; j++) {
                    if(i == 0 || i == ALTURA - 1 || j == 0 || j == LARGURA - 1)
                        tela[i][j] = PAREDE;
                    else
                        tela[i][j] = ' ';
                }
            }

            // Adiciona os segmentos da cobrinha na tela
            Node* atual = cobrinha.cabeca;
            while (atual != NULL) {
                tela[atual->y][atual->x] = CORPO_COBRINHA;
                atual = atual->prox;
            }

            // Gera a posição da comida
            if (comidaX == 0 && comidaY == 0) {
                do {
                    comidaX = rand() % (LARGURA - 2) + 1;
                    comidaY = rand() % (ALTURA - 2) + 1;
                } while (tela[comidaY][comidaX] != ' ');
            }
            tela[comidaY][comidaX] = COMIDA;

            // Imprime a tela com todos os objetos nela
            printTela(tela);

            // Imprime o relógio
            printf("Tempo: %02d:%02d\n", relogio.minutos, relogio.segundos);

            // Move a cobrinha
            Node* temp = criarNode(cobrinha.cabeca->x, cobrinha.cabeca->y);
            switch(direcao) {
                case CIMA:
                    temp->y--;
                    usleep(DELAY_VERTICAL); // Atraso para movimentos verticais
                    break;
                case BAIXO:
                    temp->y++;
                    usleep(DELAY_VERTICAL); // Atraso para movimentos verticais
                    break;
                case ESQUERDA:
                    temp->x--;
                    usleep(DELAY_HORIZONTAL); // Atraso para movimentos horizontais
                    break;
                case DIREITA:
                    temp->x++;
                    usleep(DELAY_HORIZONTAL); // Atraso para movimentos horizontais
                    break;
            }
            temp->prox = cobrinha.cabeca;
            cobrinha.cabeca = temp;

            // Checa se a cobrinha colidiu com a parede ou consigo mesma
            if(cobrinha.cabeca->x <= 0 || cobrinha.cabeca->x >= LARGURA - 1 || cobrinha.cabeca->y <= 0 || cobrinha.cabeca->y >= ALTURA - 1) {
                printf("Game Over! Score: %d\n", pontos);
                freeLista(&cobrinha);
                exit(EXIT_SUCCESS);
            }

            atual = cobrinha.cabeca->prox;
            while (atual != NULL) {
                if (atual->x == cobrinha.cabeca->x && atual->y == cobrinha.cabeca->y) {
                    printf("Game Over! Score: %d\n", pontos);
                    freeLista(&cobrinha);
                    exit(EXIT_SUCCESS);
                }
                atual = atual->prox;
            }

            // Check se a cobrinha comeu a comida
            if(cobrinha.cabeca->x == comidaX && cobrinha.cabeca->y == comidaY) {
                pontos++;
                comidaX = 0;
                comidaY = 0;
            } else if (cobrinha.cabeca != NULL) {
                Node* temp = cobrinha.cabeca;
                while (temp->prox->prox != NULL) {
                    temp = temp->prox;
                }
                free(temp->prox);
                temp->prox = NULL;
            }
        }

        pthread_cancel(thread_relogio); // Cancela a thread do relógio quando o jogo termina

        close(pipefd[0]); // Fecha a extremidade de leitura do pipe no processo pai
    }

    if(pid == 0)
        close(pipefd[1]);

    configurarTerminalPadrao(); // Restaura as configurações do terminal para o modo padrão

    return 0;
}

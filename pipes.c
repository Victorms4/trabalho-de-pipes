#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
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
    int* tempo = (int*)arg;
    while (1) {
        sleep(1);
        (*tempo)++;
    }
    return NULL;
}

int main() {
    char tela[ALTURA][LARGURA];
    char direcao;
    int comidaX = 0, comidaY = 0;
    int pontos = 0;
    int segundos = 0;

    Cobrinha cobrinha = {NULL, NULL}; // Inicializa a cobra com a cabeça e a cauda como NULL
    inicializarCobrinha(&cobrinha);
    direcao = DIREITA;

    pthread_t thread_relogio;

    if (pthread_create(&thread_relogio, NULL, atualizarRelogio, (void*)&segundos) != 0) {
        printf("Erro ao criar thread do relógio.\n");
        exit(EXIT_FAILURE);
    }

    while(1) {
        if(kbhit()) {
            direcao = getchar();
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
        printf("Tempo: %02d:%02d\n", segundos / 60, segundos % 60);

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
            pthread_cancel(thread_relogio);
            exit(EXIT_SUCCESS);
        }

        atual = cobrinha.cabeca->prox;
        while (atual != NULL) {
            if (atual->x == cobrinha.cabeca->x && atual->y == cobrinha.cabeca->y) {
                printf("Game Over! Score: %d\n", pontos);
                freeLista(&cobrinha);
                pthread_cancel(thread_relogio);
                exit(EXIT_SUCCESS);
            }
            atual = atual->prox;
        }

        // Checa se a cobrinha comeu a comida
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

    configurarTerminalPadrao(); // Restaura as configurações do terminal para o modo padrão

    return 0;
}

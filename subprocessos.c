#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

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
#define MAXBUFF 1

int i,j;

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

int kbhit(void) {
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if(ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

void printTela(char tela[ALTURA][LARGURA], char direcao) {
    system("clear");
    for(int i = 0; i < ALTURA; i++) {
        for(int j = 0; j < LARGURA; j++) {
            printf("%c", tela[i][j]);
        }
        printf("\n");
        printf("%c", direcao);
    }
}

void inicializarCobrinha(Cobrinha* cobrinha) {
    append(cobrinha, LARGURA / 2, ALTURA / 2);
    append(cobrinha, LARGURA / 2 - 1, ALTURA / 2);
    append(cobrinha, LARGURA / 2 - 2, ALTURA / 2);
}

int main() {
    char tela[ALTURA][LARGURA];
    char direcao;
    int comidaX = 0, comidaY = 0;
    int pontos = 0;
    int jogarNovamente = 1;
    int	descritor;  // usado para criar o processo filho pelo fork
	int pipe1[2];  // comunicacao pai -> filho 
    char buff[MAXBUFF];

   if (pipe(pipe1)<0)
	{ printf("Erro na chamada PIPE");
	   exit(0);
	}

   //   Fork para criar o processo filho

    if ( (descritor = fork()) <0)
	{ printf("Erro na chamada FORK");
	   exit(0);
	}

	else if (descritor >0)  // PROCESSO PAI
	   {	
        close(pipe1[0]); // fecha leitura no pipe1

        while(1){
            if(kbhit()) {
                direcao = getchar();
                buff[0] = direcao;
                write(pipe1[1], buff, 10);
            }
        }

		close(pipe1[1]); // fecha pipe1
		exit(0);

	    } // FIM DO PROCESSO PAI

	else // PROCESSO FILHO

	   {	close(pipe1[1]); // fecha escrita no pipe1


        while(jogarNovamente) {
            Cobrinha cobrinha = {NULL, NULL}; // Inicializa a cobra com a cabeça e a cauda como NULL
            inicializarCobrinha(&cobrinha);
            direcao = DIREITA;

            while(1) {

                read(pipe1[1], buff, 10);
                if(buff[0]=='a' || buff[0]=='s' || buff[0]=='d' || buff[0]=='w')
                    direcao = buff[0];

                // Inicializa a tela
                for(i = 0; i < ALTURA; i++) {
                    for(j = 0; j < LARGURA; j++) {
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
                printTela(tela, direcao);

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
            

                // Checa se a cobrinha colidiu com seu corpo ou com a parede
                if(cobrinha.cabeca->x <= 0 || cobrinha.cabeca->x >= LARGURA - 1 || cobrinha.cabeca->y <= 0 || cobrinha.cabeca->y >= ALTURA - 1) {
                    printf("Game Over! Score: %d\n", pontos);
                    freeLista(&cobrinha);
                    break;
                }

                atual = cobrinha.cabeca->prox;
                while (atual != NULL) {
                    if (atual->x == cobrinha.cabeca->x && atual->y == cobrinha.cabeca->y) {
                        printf("Game Over! Score: %d\n", pontos);
                        freeLista(&cobrinha);
                        break;
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

            printf("Deseja jogar novamente? (1 para Sim, 0 para Não): ");
            scanf("%d", &jogarNovamente);
            getchar(); // Limpar o buffer do teclado
        }

            close(pipe1[0]); // fecha leitura no pipe1
            exit(0);

	} // FIM DO PROCESSO FILHO

    return 0;
}

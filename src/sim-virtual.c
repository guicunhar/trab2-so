#include <stdio.h>
#include <stdlib.h>

// struct para representar uma página na tabela de páginas
typedef struct {
    int present;
    int frame;
    int referenced;
    int modified;
} page_struct;

// struct para representar um frame na memória física
typedef struct {
    int page;
    int free;
    int referenced;
    int modified;
    unsigned long last_access;
} frame_struct;

int main(int argc, char *argv[]) {

    int page_faults = 0; // contador de page faults

    // tratamento de erro caso a pessoa n coloque argumentos suficientes
    if (argc != 5) {
        printf("Uso: ./sim-virtual <algoritmo> <arquivo.log> <tamanho_pagina_kb> <memoria_mb>\n");
        return 1;
    }

    // leitura dos argumentos e tratamento de erros
    char *algoritmo = argv[1];
    char *arquivo = argv[2];
    int page_kb = atoi(argv[3]);
    int mem_mb = atoi(argv[4]);

    if (page_kb != 8 && page_kb != 16 && page_kb != 32) {
        printf("Tamanho da página deve ser 8, 16 ou 32.\n");
        return 1;
    }

    // calcular o shift baseado no tamanho da página
    int s;
    if (page_kb == 8) s = 13;
    else if (page_kb == 16) s = 14;
    else s = 15;

    if (mem_mb != 1 && mem_mb != 2 && mem_mb != 4) {
        printf("Tamanho da memória deve ser 1, 2 ou 4.\n");
        return 1;
    }

    // calcular o número de frames
    int frame_size = page_kb * 1024;
    int mem_size = mem_mb * 1024 * 1024;
    int nframes = mem_size / frame_size;

    printf("Total de frames = %d\n", nframes);

    // alocando memoria e criando a tabela de páginas
    page_struct *page_table = calloc(2000000, sizeof(page_struct));

    // alocando memoria e criando o array de frames
    frame_struct *frames = calloc(nframes, sizeof(frame_struct));

    // inicializando os frames como livres
    for (int i = 0; i < nframes; i++)
        frames[i].free = 1;

    // abre o arquivo
    FILE *f = fopen(arquivo, "r");
    if (!f) {
        printf("Erro ao abrir arquivo.\n");
        return 1;
    }

    // leitura do primeiro endereço e operação do arquivo
    unsigned int addr;
    char op;

    while (fscanf(f, "%x %c", &addr, &op) == 2) {

        // converter endereço lógico na pagina
        unsigned int page = addr >> s;
        printf("Endereço: %x, Operação: %c -> Página %u\n", addr, op, page);

        // verificar se a página está presente na memória
        if (page_table[page].present) {

            // HIT
            page_table[page].referenced = 1;
            if (op == 'W')
                page_table[page].modified = 1;

        } else {

            // FAULT
            page_faults++;
            printf("PAGE FAULT na página %u\n", page);

            // procurar um frame livre
            int frame_encontrado = -1;
            for (int i = 0; i < nframes; i++) {
                if (frames[i].free == 1) {
                    frame_encontrado = i;
                    break;
                }
            }

            if (frame_encontrado == -1) {
                // NÃO TEM FRAME LIVRE (IMPLEMENTAR SUBSTITUIÇÃO AQUI)
                printf("Não há frames livres — implementar LRU/NRU/ÓTIMO aqui depois.\n");
                break;
            }

            // carregar a página no frame encontrado
            printf("Carregando página %u no frame %d\n", page, frame_encontrado);

            frames[frame_encontrado].free = 0;
            frames[frame_encontrado].page = page;
            frames[frame_encontrado].referenced = 1;
            frames[frame_encontrado].modified = (op == 'W');

            // atualizar a tabela de páginas
            page_table[page].present = 1;
            page_table[page].frame = frame_encontrado;
            page_table[page].referenced = 1;
            page_table[page].modified = (op == 'W');
        }

        break; 
    }

    fclose(f);

    return 0;
}

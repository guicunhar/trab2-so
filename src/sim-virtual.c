#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>  
#include <limits.h>

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

int lru_substituicao(frame_struct *frames, int nframes) {
    int frame_vitima = 0;
    unsigned long menor_tempo = frames[0].last_access;
    
    for (int i = 1; i < nframes; i++) {
        if (frames[i].last_access < menor_tempo) {
            menor_tempo = frames[i].last_access;
            frame_vitima = i;
        }
    }
    
    return frame_vitima;
}

int nru_substituicao(frame_struct *frames, int nframes) {
    int classe_vitima = 4;
    int frame_vitima = 0;
    
    for (int i = 0; i < nframes; i++) {
        int classe;
        if (frames[i].referenced == 0 && frames[i].modified == 0)
            classe = 0;
        else if (frames[i].referenced == 0 && frames[i].modified == 1)
            classe = 1;
        else if (frames[i].referenced == 1 && frames[i].modified == 0)
            classe = 2;
        else
            classe = 3;
        
        if (classe < classe_vitima) {
            classe_vitima = classe;
            frame_vitima = i;
        }
    }
    
    return frame_vitima;
}

int otimo_substituicao(frame_struct *frames, int nframes, unsigned int *sequencia_acessos,
                        unsigned long posicao_atual, unsigned long total_acessos) {
    int frame_vitima = 0;
    unsigned long maior_distancia = 0;

    unsigned long *proximo_acesso = calloc(nframes, sizeof(unsigned long));

    for (int i = 0; i < nframes; i++) {
        proximo_acesso[i] = ULONG_MAX;
    }

    for (unsigned long pos = posicao_atual + 1; pos < total_acessos; pos++) {
        unsigned int page_futura = sequencia_acessos[pos];

        for (int i = 0; i < nframes; i++) {
            if ((unsigned int)frames[i].page == page_futura && proximo_acesso[i] == ULONG_MAX) {
                proximo_acesso[i] = pos - posicao_atual;
            }
        }

        int todos_encontrados = 1;
        for (int i = 0; i < nframes; i++) {
            if (proximo_acesso[i] == ULONG_MAX) {
                todos_encontrados = 0;
                break;
            }
        }

        if (todos_encontrados) {
            break;
        }
    }

    for (int i = 0; i < nframes; i++) {
        if (proximo_acesso[i] > maior_distancia) {
            maior_distancia = proximo_acesso[i];
            frame_vitima = i;
        }
    }

    free(proximo_acesso);

    return frame_vitima;
}

int main(int argc, char *argv[]) {
    int page_faults = 0;
    int paginas_escritas = 0;
    unsigned long time = 0;

    if (argc != 5) {
        printf("Uso: ./sim-virtual <algoritmo> <arquivo.log> <tamanho_pagina_kb> <memoria_mb>\n");
        return 1;
    }

    char *algoritmo = argv[1];
    char *arquivo = argv[2];
    int page_kb = atoi(argv[3]);
    int mem_mb = atoi(argv[4]);

    if (strcasecmp(algoritmo, "LRU") != 0 && strcasecmp(algoritmo, "NRU") != 0 &&
        strcasecmp(algoritmo, "ótimo") != 0 && strcasecmp(algoritmo, "otimo") != 0) {
        printf("Algoritmo deve ser LRU, NRU ou ótimo.\n");
        return 1;
    }

    if (page_kb != 8 && page_kb != 16 && page_kb != 32) {
        printf("Tamanho da página deve ser 8, 16 ou 32.\n");
        return 1;
    }

    int s;
    if (page_kb == 8) s = 13;
    else if (page_kb == 16) s = 14;
    else s = 15;

    if (mem_mb != 1 && mem_mb != 2 && mem_mb != 4) {
        printf("Tamanho da memória deve ser 1, 2 ou 4.\n");
        return 1;
    }

    int frame_size = page_kb * 1024;
    int mem_size = mem_mb * 1024 * 1024;
    int nframes = mem_size / frame_size;

    #define MAX_PAGES 2000000
    page_struct *page_table = calloc(MAX_PAGES, sizeof(page_struct));
    frame_struct *frames = calloc(nframes, sizeof(frame_struct));

    for (int i = 0; i < nframes; i++)
        frames[i].free = 1;

    FILE *f = fopen(arquivo, "r");
    if (!f) {
        printf("Erro ao abrir arquivo.\n");
        return 1;
    }

    printf("Executando o simulador...\n");

    unsigned int *sequencia_paginas = NULL;
    char *sequencia_ops = NULL;
    unsigned long total_acessos = 0;

    if (strcasecmp(algoritmo, "otimo") == 0 || strcasecmp(algoritmo, "ótimo") == 0) {
        printf("Pré-processando arquivo para algoritmo ótimo...\n");

        unsigned int temp_addr;
        char temp_op;
        while (fscanf(f, "%x %c", &temp_addr, &temp_op) == 2) {
            total_acessos++;
        }

        printf("Total de acessos: %lu\n", total_acessos);

        sequencia_paginas = malloc(total_acessos * sizeof(unsigned int));
        sequencia_ops = malloc(total_acessos * sizeof(char));

        rewind(f);
        unsigned long idx = 0;
        while (fscanf(f, "%x %c", &temp_addr, &temp_op) == 2) {
            sequencia_paginas[idx] = temp_addr >> s;
            sequencia_ops[idx] = temp_op;
            idx++;
        }

        fclose(f);
        printf("Pré-processamento concluído!\n");
    }

    unsigned int addr;
    char op;
    unsigned long posicao_atual = 0;

    int continuar = 1;
    while (continuar) {
        if (sequencia_paginas != NULL) {
            if (posicao_atual >= total_acessos) {
                break;
            }
            addr = sequencia_paginas[posicao_atual] << s;
            op = sequencia_ops[posicao_atual];
        } else {
            if (fscanf(f, "%x %c", &addr, &op) != 2) {
                break;
            }
        }

        time++;

        unsigned int page = addr >> s;

        if (sequencia_paginas != NULL) {
            page = sequencia_paginas[posicao_atual];
        }

        if (page >= MAX_PAGES) {
            fprintf(stderr, "ERRO: Número de página %u excede limite da tabela (%d)\n", page, MAX_PAGES);
            posicao_atual++;
            continue; 
        }

        if (page_table[page].present) {
            int frame_idx = page_table[page].frame;
            page_table[page].referenced = 1;
            frames[frame_idx].referenced = 1;
            frames[frame_idx].last_access = time;
            
            if (op == 'W') {
                page_table[page].modified = 1;
                frames[frame_idx].modified = 1;
            }
        } else {
            page_faults++;
            
            int frame_encontrado = -1;
            for (int i = 0; i < nframes; i++) {
                if (frames[i].free == 1) {
                    frame_encontrado = i;
                    break;
                }
            }

            if (frame_encontrado == -1) {
                int frame_vitima;

                if (strcasecmp(algoritmo, "LRU") == 0) {
                    frame_vitima = lru_substituicao(frames, nframes);
                } else if (strcasecmp(algoritmo, "NRU") == 0) {
                    frame_vitima = nru_substituicao(frames, nframes);
                } else {
                    frame_vitima = otimo_substituicao(frames, nframes, sequencia_paginas, posicao_atual, total_acessos);
                }
                
                int pagina_vitima = frames[frame_vitima].page;
                
                if (frames[frame_vitima].modified == 1) {
                    paginas_escritas++;
                }
                
                page_table[pagina_vitima].present = 0;
                frame_encontrado = frame_vitima;
            }

            if (frame_encontrado != -1) {
                frames[frame_encontrado].free = 0;
                frames[frame_encontrado].page = page;
                frames[frame_encontrado].referenced = 1;
                frames[frame_encontrado].modified = (op == 'W');
                frames[frame_encontrado].last_access = time;

                page_table[page].present = 1;
                page_table[page].frame = frame_encontrado;
                page_table[page].referenced = 1;
                page_table[page].modified = (op == 'W');
            }
        }

        if (strcasecmp(algoritmo, "NRU") == 0 && time % 10 == 0) {
            for (int i = 0; i < nframes; i++) {
                if (!frames[i].free) {
                    frames[i].referenced = 0;
                    int page_idx = frames[i].page;
                    page_table[page_idx].referenced = 0;
                }
            }
        }

        posicao_atual++;
    }

    if (sequencia_paginas == NULL) {
        fclose(f);
    }

    if (sequencia_paginas != NULL) {
        free(sequencia_paginas);
        free(sequencia_ops);
    }

    printf("Arquivo de entrada: %s\n", arquivo);
    printf("Tamanho da memoria física: %d MB\n", mem_mb);
    printf("Tamanho das páginas: %d KB\n", page_kb);
    printf("Algoritmo de substituição: %s\n", algoritmo);
    printf("Número de Faltas de Páginas: %d\n", page_faults);
    printf("Número de Páginas Escritas: %d\n", paginas_escritas);

    free(page_table);
    free(frames);

    return 0;
}

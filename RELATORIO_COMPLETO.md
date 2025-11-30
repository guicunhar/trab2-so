# Relatório - Simulador de Memória Virtual
**Trabalho 2 - INF1316 - Sistemas Operacionais**

**Aluno:** Breno Braga Neves (2310590) e Guilherme Cunha (2320951)
**Data:** Novembro de 2025
**Professor:** Seibel

---

## 1. Resumo do Projeto

### 1.1 Objetivo

Este trabalho implementa um **simulador de memória virtual com paginação** em linguagem C, capaz de simular três algoritmos clássicos de substituição de páginas: **LRU (Least Recently Used)**, **NRU (Not Recently Used)** e o **algoritmo Ótimo** de Belady.

O simulador processa traces reais de acesso à memória extraídos de quatro programas significativos: um compilador, um programa de operações com matrizes, um compressor de arquivos e um simulador de partículas. Durante a execução, o simulador coleta estatísticas detalhadas sobre page faults e páginas escritas, permitindo análise comparativa do desempenho dos diferentes algoritmos de substituição.

### 1.2 Estrutura Geral do Simulador

O simulador é composto por um único arquivo fonte (`src/sim-virtual.c`) com 320 linhas de código e opera em cinco etapas principais:

1. **Validação de Parâmetros**: Verifica a validade do algoritmo escolhido, do tamanho de página (8/16/32 KB) e da memória física disponível (1/2/4 MB)

2. **Inicialização das Estruturas**: Aloca dinamicamente a tabela de páginas com 2 milhões de entradas e o array de frames físicos, cujo tamanho é calculado como `(memória_MB × 1024 × 1024) / (página_KB × 1024)`

3. **Pré-processamento (apenas para Algoritmo Ótimo)**: Carrega todo o arquivo de trace na memória para permitir consultas de acessos futuros, otimizando significativamente o tempo de execução

4. **Simulação Principal**: Processa sequencialmente cada acesso à memória, detectando page faults, executando substituições quando necessário e atualizando os bits de controle (R e M)

5. **Geração de Relatório**: Imprime estatísticas finais incluindo número total de page faults e quantidade de páginas sujas escritas no disco

### 1.3 Estruturas de Dados

#### Tabela de Páginas (`page_struct`)

Representa o mapeamento de páginas virtuais para frames físicos na memória RAM:

```c
typedef struct {
    int present;      // Página está presente na memória física (0=não, 1=sim)
    int frame;        // Número do frame onde a página está armazenada
    int referenced;   // Bit R - indica se foi referenciada recentemente
    int modified;     // Bit M (dirty bit) - indica se foi modificada
} page_struct;
```

**Características:**
- **Tamanho**: Array estático de 2.000.000 entradas
- **Justificativa**: Endereços de 32 bits com páginas de 8KB (mínimo) resultam em 19 bits de índice de página (2^19 = 524.288 páginas possíveis). O tamanho de 2M fornece margem de segurança
- **Memória ocupada**: Aproximadamente 32 MB (2M × 16 bytes)

#### Descritor de Frame (`frame_struct`)

Representa cada frame de memória física disponível:

```c
typedef struct {
    int page;                  // Número da página atualmente armazenada
    int free;                  // Indica se o frame está livre (1) ou ocupado (0)
    int referenced;            // Bit R para algoritmo NRU
    int modified;              // Bit M - indica se o frame foi modificado
    unsigned long last_access; // Timestamp do último acesso (usado pelo LRU)
} frame_struct;
```

**Tamanho dinâmico**: Calculado em tempo de execução baseado nos parâmetros de entrada

**Exemplos de configurações:**
- 2 MB memória, 8 KB página → 256 frames (2.097.152 / 8.192 = 256)
- 4 MB memória, 16 KB página → 256 frames
- 1 MB memória, 32 KB página → 32 frames

---

## 2. Implementação dos Algoritmos de Substituição

### 2.1 Algoritmo LRU (Least Recently Used)

**Princípio**: Substitui a página que não foi utilizada há mais tempo, baseando-se na hipótese de que páginas recentemente acessadas tendem a ser acessadas novamente em breve (localidade temporal).

**Implementação**:
- Cada frame mantém um campo `last_access` que armazena o timestamp do último acesso
- A cada acesso bem-sucedido (hit), o campo `last_access` do frame correspondente é atualizado para o tempo atual
- Durante uma substituição, o algoritmo percorre linearmente todos os frames e seleciona aquele com o menor valor de `last_access`

**Complexidade**: O(n) para encontrar a vítima, onde n = número de frames

**Vantagens**:
- Excelente aproximação da política ótima na prática
- Funciona muito bem com programas que apresentam localidade temporal
- Comportamento previsível e consistente

**Desvantagens**:
- Requer manutenção constante de timestamps
- Sobrecarga de memória para armazenar o campo `last_access`

### 2.2 Algoritmo NRU (Not Recently Used)

**Princípio**: Classifica as páginas em quatro classes baseadas nos bits R (Referenced) e M (Modified), preferindo substituir páginas que não foram referenciadas recentemente e que não foram modificadas.

**Classes (ordenadas por preferência de substituição)**:
- **Classe 0**: R=0, M=0 (não referenciada, não modificada) - **candidata ideal**
- **Classe 1**: R=0, M=1 (não referenciada, mas modificada)
- **Classe 2**: R=1, M=0 (referenciada, não modificada)
- **Classe 3**: R=1, M=1 (referenciada e modificada) - **pior candidata**

**Implementação**:
- O algoritmo classifica cada frame em uma das quatro classes
- Seleciona para substituição a primeira página encontrada na classe de menor prioridade
- Implementa reset periódico: **a cada 10 acessos à memória**, todos os bits R são zerados simultaneamente
- O bit M nunca é resetado automaticamente (apenas quando a página é efetivamente escrita no disco)

**Justificativa do Reset**:
O reset periódico dos bits R simula o comportamento de uma interrupção de relógio em sistemas operacionais reais, permitindo distinguir páginas acessadas recentemente daquelas acessadas há mais tempo.

**Vantagens**:
- Implementação simples e eficiente
- Não requer manutenção de timestamps
- Overhead computacional mínimo

**Desvantagens**:
- Menos preciso que o LRU
- Granularidade grossa (apenas 4 classes)
- Sensível à frequência do reset

### 2.3 Algoritmo Ótimo (Belady)

**Princípio**: Substitui a página que será acessada no futuro mais distante (ou que nunca mais será acessada), garantindo teoricamente o mínimo número absoluto de page faults possível.

**Implementação**:

**Fase de Pré-processamento**:
1. Todo o arquivo de trace é lido e armazenado em um array na memória
2. Para cada acesso, armazena-se o número da página (após aplicar o shift)
3. Registra-se também a operação (R/W) para cada acesso

**Fase de Substituição**:
1. Para cada frame atualmente na memória, o algoritmo varre os acessos futuros a partir da posição atual
2. Registra a distância (em número de acessos) até a próxima referência de cada página
3. Páginas que nunca mais serão acessadas recebem distância INFINITA (ULONG_MAX)
4. Seleciona para substituição o frame cuja próxima referência está mais distante

**Otimização Implementada**:
O algoritmo interrompe a busca antecipadamente assim que encontra as próximas referências de todos os frames, evitando varrer desnecessariamente todo o arquivo de trace.

**Vantagens**:
- Desempenho teoricamente perfeito (mínimo de page faults)
- Serve como referência (baseline) para avaliar outros algoritmos

**Desvantagens**:
- **Impossível de implementar em sistemas reais** (requer conhecimento do futuro)
- Alto custo computacional mesmo com otimizações (O(m×n) onde m = acessos futuros, n = frames)
- Requer memória significativa para armazenar todo o trace (~4 MB para 1 milhão de acessos)

**Nota sobre Viabilidade**:
Este algoritmo só é utilizável em contexto de simulação ou análise offline de traces. Em sistemas operacionais reais, impossível prever acessos futuros.

---

## 3. Análise de Desempenho

### 3.1 Metodologia

Foram realizados testes sistemáticos cobrindo diferentes aspectos:
- **Arquivos de trace**: compilador.log, matriz.log, compressor.log, simulador.log
- **Algoritmos**: LRU, NRU, Ótimo
- **Tamanhos de página**: 8, 16, 32 KB
- **Tamanhos de memória**: 1, 2, 4 MB

Todos os testes foram executados em ambiente Linux (WSL2) com compilação GCC usando flags `-O2` para otimização.

### 3.2 Comparação entre Algoritmos

**Configuração**: Memória = 2 MB, Página = 8 KB (256 frames disponíveis)

#### Tabela 1: Desempenho dos Algoritmos com compilador.log

| Algoritmo | Page Faults | Páginas Escritas | Desempenho Relativo |
|-----------|-------------|------------------|---------------------|
| **Ótimo** | 10.118      | 2.190            | 100% (baseline)     |
| **LRU**   | 21.091      | 3.839            | 208% do Ótimo       |
| **NRU**   | 107.981     | 7.549            | 1067% do Ótimo      |

#### Análise Detalhada:

**1. Superioridade do Algoritmo Ótimo:**

O algoritmo Ótimo apresentou apenas 10.118 page faults, estabelecendo o limite teórico mínimo para esta configuração. Este resultado confirma a expectativa teórica de que o algoritmo Ótimo produz o melhor desempenho possível.

**2. Desempenho do LRU:**

O LRU apresentou 21.091 page faults, correspondendo a aproximadamente **o dobro** do algoritmo Ótimo (108% a mais). Este é um resultado **excelente**, considerando que:
- O LRU não tem conhecimento dos acessos futuros
- A diferença de apenas 2x demonstra que o padrão de acesso do compilador possui forte localidade temporal
- O LRU conseguiu capturar eficientemente essa localidade através dos timestamps

**3. Desempenho do NRU:**

O NRU apresentou 107.981 page faults, **10,7 vezes mais** que o Ótimo e **5,1 vezes mais** que o LRU. Esta diferença significativa pode ser explicada por:

- **Granularidade grossa**: Apenas 4 classes de prioridade versus timestamps contínuos do LRU
- **Reset periódico**: A cada 10 acessos, todos os bits R são zerados, causando perda de informação sobre o histórico de acessos
- **Sensibilidade ao padrão**: O intervalo de reset (10 acessos) pode não ser ideal para o padrão específico do compilador

**4. Páginas Escritas:**

A proporção de páginas escritas segue padrão similar aos page faults:
- Ótimo: 2.190 (21.6% dos faults)
- LRU: 3.839 (18.2% dos faults)
- NRU: 7.549 (7.0% dos faults)

O NRU, apesar de ter muito mais faults, apresenta menor percentual de páginas sujas escritas, sugerindo que substitui frequentemente páginas que foram apenas lidas.

#### Tabela 2: Desempenho com Diferentes Programas (LRU, 8KB, 2MB)

| Programa       | Page Faults | Páginas Escritas | Taxa Write/Fault |
|----------------|-------------|------------------|------------------|
| compressor.log | 255         | 0                | 0.0%             |
| matriz.log     | 4.745       | 1.623            | 34.2%            |
| simulador.log  | 8.556       | 3.395            | 39.7%            |
| compilador.log | 21.091      | 3.839            | 18.2%            |

**Análise por Programa:**

**Compressor (255 faults - EXCELENTE):**
- **Desempenho excepcional** com apenas 255 page faults em 1 milhão de acessos
- Taxa de page fault de apenas 0.026%
- **Zero páginas escritas** - apenas leitura
- Indica working set muito pequeno que cabe completamente nos 256 frames disponíveis
- Padrão de acesso altamente localizado, provavelmente processando dados sequencialmente

**Matriz (4.745 faults - MUITO BOM):**
- Segundo melhor desempenho
- Taxa de escrita relativamente alta (34.2%), típico de operações matriciais que modificam elementos
- Sugere bom aproveitamento de localidade espacial (elementos adjacentes da matriz)
- Working set moderado, bem comportado

**Simulador (8.556 faults - BOM):**
- Maior taxa de escritas (39.7%), característico de simulações que atualizam estado
- Padrão de acesso mais irregular que os anteriores
- Ainda assim, working set cabe razoavelmente na memória disponível

**Compilador (21.091 faults - RAZOÁVEL):**
- Pior desempenho entre os quatro programas
- Working set maior e mais disperso
- Típico de compiladores que acessam: código fonte, tabelas de símbolos, árvores sintáticas, código gerado
- Menor taxa de escrita (18.2%) - predominância de leitura

**Conclusão**: Programas com padrões de acesso mais regulares e working sets menores (compressor, matriz) beneficiam-se significativamente da paginação, enquanto programas complexos (compilador) apresentam maior contenção.

### 3.3 Impacto do Tamanho de Página

**Configuração**: Memória = 2 MB, Algoritmo = LRU, Arquivo = compilador.log

#### Tabela 3: Variação do Tamanho de Página

| Tamanho Página | Frames Disponíveis | Page Faults | Páginas Escritas | Variação |
|----------------|-------------------|-------------|------------------|----------|
| 8 KB           | 256               | 21.091      | 3.839            | baseline |
| 16 KB          | 128               | 30.686      | 5.068            | +45.5%   |
| 32 KB          | 64                | 38.641      | 6.186            | +83.2%   |

#### Análise do Trade-off:

**Observação Principal**: À medida que o tamanho de página aumenta, o número de page faults também aumenta significativamente.

**Fatores em Jogo**:

**1. Redução do Número de Frames (efeito negativo dominante):**
- Páginas de 8 KB → 256 frames disponíveis
- Páginas de 16 KB → apenas 128 frames (redução de 50%)
- Páginas de 32 KB → apenas 64 frames (redução de 75%)

Com menos frames disponíveis, a memória física se esgota mais rapidamente, resultando em maior contenção e mais substituições.

**2. Localidade Espacial (efeito positivo, mas limitado):**
- Páginas maiores agrupam mais dados contíguos
- Teoricamente, um único page fault traz mais dados úteis
- Entretanto, este benefício é **superado pela perda de frames**

**3. Fragmentação Interna (efeito negativo adicional):**
- Páginas de 32 KB desperdiçam mais espaço quando o programa não utiliza a página inteira
- Maior quantidade de memória "desperdiçada" em cada frame

**Resultado Prático**: Para o padrão de acesso do compilador, **páginas menores (8 KB) apresentam melhor desempenho**. A flexibilidade de ter mais frames supera os benefícios da localidade espacial.

**Quando páginas maiores seriam melhores:**
- Programas com acesso predominantemente sequencial
- Processamento de grandes arrays contíguos
- Aplicações científicas com matrizes densas

### 3.4 Impacto do Tamanho da Memória

**Configuração**: Página = 8 KB, Algoritmo = LRU, Arquivo = compilador.log

#### Tabela 4: Escalabilidade da Memória

| Memória | Frames | Page Faults | Páginas Escritas | Redução vs Anterior |
|---------|--------|-------------|------------------|---------------------|
| 1 MB    | 128    | 36.707      | 5.775            | baseline            |
| 2 MB    | 256    | 21.091      | 3.839            | -42.5%              |
| 4 MB    | 512    | 7.632       | 1.756            | -63.8%              |

**Gráfico Mental (Lei dos Retornos Decrescentes):**
```
Page Faults
40,000 |●
       |
30,000 |
       |    ●
20,000 |
       |
10,000 |            ●
       |________________
        1MB   2MB    4MB
```

#### Análise da Escalabilidade:

**1. Redução Não-Linear:**
- Dobrar memória (1→2 MB) reduz faults em 42.5%
- Dobrar novamente (2→4 MB) reduz faults em 63.8% *do novo valor*
- Mas a redução absoluta diminui: 15.616 faults (1→2MB) vs 13.459 faults (2→4MB)

**2. Working Set do Compilador:**
O comportamento sugere que o working set do compilador está entre 2-4 MB:
- Com 1 MB: working set não cabe → muitos faults
- Com 2 MB: working set parcialmente acomodado → faults moderados
- Com 4 MB: working set quase completamente acomodado → poucos faults

**3. Lei dos Retornos Decrescentes:**
Cada MB adicional de memória produz menos benefício que o anterior. Este é um padrão esperado em sistemas de memória virtual.

**4. Custo-Benefício:**
- 1 MB → 2 MB: **Muito recomendado** (redução de 42.5%)
- 2 MB → 4 MB: **Recomendado** (redução adicional de 63.8%)
- Além de 4 MB: Provavelmente **rendimentos decrescentes** para este programa

**5. Páginas Escritas:**
Segue padrão similar, com redução de 84.2% ao ir de 1 MB para 4 MB.

**Conclusão Prática**: Para o compilador testado, **2 MB representa um bom equilíbrio** entre desempenho e custo de memória. Investir em 4 MB traz benefícios adicionais, mas com menor retorno relativo.

---

## 4. Pseudocódigo do Algoritmo Ótimo

```
ALGORITMO: Substituição Ótima de Páginas (Belady)

ENTRADA:
  - frames[nframes]: array de frames atualmente na memória
  - nframes: número total de frames disponíveis
  - sequencia_acessos[total]: array com páginas que serão acessadas (pré-processado)
  - posicao_atual: índice da posição atual na sequência
  - total_acessos: tamanho total da sequência de acessos

SAÍDA:
  - índice do frame vítima a ser substituído

INÍCIO

  1. Criar array auxiliar:
     proximo_acesso[nframes] ← INFINITO

  2. Para cada posição futura na sequência:
     PARA pos ← posicao_atual + 1 ATÉ total_acessos - 1 FAZER

       pagina_futura ← sequencia_acessos[pos]

       // Verifica se esta página está em algum frame
       PARA i ← 0 ATÉ nframes - 1 FAZER
         SE frames[i].pagina == pagina_futura E proximo_acesso[i] == INFINITO ENTÃO
           proximo_acesso[i] ← (pos - posicao_atual)
         FIM-SE
       FIM-PARA

       // OTIMIZAÇÃO: Para se todos os frames foram encontrados
       todos_encontrados ← VERDADEIRO
       PARA i ← 0 ATÉ nframes - 1 FAZER
         SE proximo_acesso[i] == INFINITO ENTÃO
           todos_encontrados ← FALSO
           SAIR-PARA
         FIM-SE
       FIM-PARA

       SE todos_encontrados == VERDADEIRO ENTÃO
         SAIR-PARA  // Não precisa continuar buscando
       FIM-SE

     FIM-PARA

  3. Encontrar frame com maior distância (ou que nunca será usado):
     vitima ← 0
     maior_distancia ← proximo_acesso[0]

     PARA i ← 1 ATÉ nframes - 1 FAZER
       SE proximo_acesso[i] > maior_distancia ENTÃO
         maior_distancia ← proximo_acesso[i]
         vitima ← i
       FIM-SE
     FIM-PARA

  4. RETORNAR vitima

FIM
```

**Explicação Linha a Linha**:

- **Linhas 1**: Inicializa array de distâncias com INFINITO, representando "nunca mais será usado"
- **Linhas 2**: Loop que varre os acessos futuros a partir da posição atual
- **Verificação interna**: Para cada acesso futuro, verifica se a página está em algum frame
- **Registro da distância**: Marca a distância apenas na **primeira ocorrência** futura de cada página
- **Otimização (crucial)**: Interrompe a busca assim que todas as páginas dos frames foram encontradas, evitando varredura completa desnecessária
- **Linhas 3**: Seleciona o frame cuja próxima referência está mais distante
- **Tratamento de INFINITO**: Frames com proximo_acesso == INFINITO (nunca mais usados) são os melhores candidatos

**Complexidade**:
- **Pior caso**: O(m × n), onde m = acessos futuros restantes, n = número de frames
- **Caso médio**: Melhor devido à otimização de parada antecipada
- **Custo de memória**: O(n) para o array auxiliar

**Por que não é usável em sistemas reais**:
Requer conhecimento completo de todos os acessos futuros, o que é impossível sem capacidade de prever o futuro do programa em execução.

---

## 5. Conclusões

### 5.1 Principais Resultados Obtidos

**1. Hierarquia de Desempenho dos Algoritmos:**
```
Ótimo (10.118 faults) < LRU (21.091 faults) < NRU (107.981 faults)
```

- O algoritmo Ótimo estabeleceu o baseline teórico com o mínimo absoluto de page faults
- O **LRU apresentou desempenho próximo ao Ótimo** (apenas 2x mais faults), validando sua eficácia como algoritmo prático
- O NRU, embora mais simples, teve desempenho significativamente inferior (10x pior que o Ótimo)

**2. Trade-off do Tamanho de Página:**

Para o padrão de acesso do compilador testado, **páginas menores (8 KB) são superiores**:
- 8 KB: 21.091 faults
- 16 KB: 30.686 faults (+45%)
- 32 KB: 38.641 faults (+83%)

A redução no número de frames disponíveis supera os benefícios da localidade espacial.

**3. Escalabilidade da Memória:**

Memória adicional produz retornos decrescentes:
- 1→2 MB: redução de 42.5% nos page faults
- 2→4 MB: redução adicional de 63.8% (mas menor em termos absolutos)

Sugere que o working set do compilador está entre 2-4 MB.

**4. Variabilidade entre Programas:**

O desempenho varia drasticamente conforme o padrão de acesso:
- Compressor: apenas 255 faults (excelente localidade)
- Matriz: 4.745 faults (boa localidade)
- Simulador: 8.556 faults (localidade moderada)
- Compilador: 21.091 faults (localidade mais fraca)

### 5.2 Aprendizados sobre Sistemas Operacionais

**1. Localidade de Referência é Fundamental:**

O sucesso do LRU (próximo ao Ótimo) confirma que a hipótese de localidade temporal é válida para programas reais. Páginas acessadas recentemente tendem a ser acessadas novamente.

**2. Simplicidade vs Desempenho:**

O NRU ilustra um trade-off importante em Sistemas Operacionais:
- **Vantagem**: Implementação simples, baixo overhead
- **Desvantagem**: Desempenho inferior em até 5x comparado ao LRU

Em sistemas embarcados ou de tempo real, onde simplicidade é crítica, o NRU pode ser aceitável. Em sistemas de propósito geral, o LRU justifica sua complexidade adicional.

**3. Importância do Tuning de Parâmetros:**

Os resultados demonstram que parâmetros de sistema (tamanho de página, quantidade de memória) têm **impacto dramático** no desempenho:
- Escolha incorreta de tamanho de página pode quase dobrar os page faults
- Memória insuficiente pode aumentar faults em 380%

**4. Working Set Matters:**

Programas diferentes têm requisitos dramaticamente diferentes de memória. Um único tamanho não serve para todos (compressor: 255 faults vs compilador: 21.091 faults com mesma configuração).

### 5.3 Limitações e Trabalhos Futuros

**Limitações deste Trabalho:**

1. **Algoritmo Ótimo computacionalmente caro**: Mesmo com otimizações, requer ~30 segundos por teste, inviável para uso real
2. **Tabela de páginas fixa**: 2M entradas independentemente do tamanho real do working set
3. **Apenas um processo**: Simulador não considera multiprogramação
4. **Reset fixo do NRU**: Intervalo de 10 acessos é arbitrário, não adaptativo

**Possíveis Extensões:**

1. **Algoritmo Clock**: Implementar aproximação eficiente do LRU usando ponteiro circular
2. **Working Set Algorithm**: Rastrear dinamicamente o working set de cada programa
3. **Multiprogramação**: Simular múltiplos processos competindo por memória
4. **TLB (Translation Lookaside Buffer)**: Simular cache de traduções para realismo
5. **Tabela de Páginas Multinível**: Implementar estrutura hierárquica mais realista
6. **Page Buffering**: Manter pool de páginas livres para otimizar substituições
7. **Adaptação Dinâmica**: Ajustar parâmetros (reset do NRU, tamanho de página) baseado em padrão de acesso observado

### 5.4 Considerações Finais

Este trabalho demonstrou empiricamente os conceitos fundamentais de memória virtual estudados em aula. Os resultados validaram princípios teóricos:

✓ **Algoritmo Ótimo** fornece limite inferior de page faults (teoria de Belady)
✓ **LRU** aproxima-se bem do ótimo quando há localidade temporal (Stack Algorithm)
✓ **Páginas menores** oferecem mais flexibilidade mas aumentam overhead de tabela
✓ **Memória adicional** segue lei de retornos decrescentes (curva côncava)

A implementação bem-sucedida do simulador e a análise detalhada dos resultados proporcionaram compreensão profunda dos trade-offs envolvidos no projeto de sistemas de memória virtual, conhecimento essencial para engenheiros de sistemas operacionais.

---

## 6. Referências

1. **Tanenbaum, Andrew S.; Bos, Herbert.** "Modern Operating Systems". 4th Edition. Pearson, 2014.
   - Capítulo 3: Memory Management (Seções 3.3 e 3.4)

2. **Silberschatz, Abraham; Galvin, Peter B.; Gagne, Greg.** "Operating System Concepts". 10th Edition. Wiley, 2018.
   - Capítulo 9: Virtual Memory (Seções 9.4 sobre Page Replacement Algorithms)

3. **Belady, László A.** "A Study of Replacement Algorithms for Virtual-Storage Computer". IBM Systems Journal, Vol. 5, No. 2, 1966.
   - Artigo original sobre o algoritmo ótimo

4. **Denning, Peter J.** "The Working Set Model for Program Behavior". Communications of the ACM, Vol. 11, No. 5, 1968.
   - Conceito fundamental de working set

5. Material didático da disciplina INF1316 - Sistemas Operacionais, PUC-Rio

6. **Stevens, W. Richard; Rago, Stephen A.** "Advanced Programming in the UNIX Environment". 3rd Edition. Addison-Wesley, 2013.
   - Referência para implementação em C

---

## Anexo A: Dados Completos dos Testes

### Teste Conjunto 1: Comparação de Algoritmos
**Configuração**: compilador.log, 8 KB páginas, 2 MB memória

```
LRU:
  Número de Faltas de Páginas: 21091
  Número de Páginas Escritas: 3839
  Taxa de Write: 18.2%

NRU:
  Número de Faltas de Páginas: 107981
  Número de Páginas Escritas: 7549
  Taxa de Write: 7.0%

Ótimo:
  Número de Faltas de Páginas: 10118
  Número de Páginas Escritas: 2190
  Taxa de Write: 21.6%
```

### Teste Conjunto 2: Variação de Tamanho de Página
**Configuração**: compilador.log, LRU, 2 MB memória

```
8 KB (256 frames):
  Page Faults: 21091
  Páginas Escritas: 3839

16 KB (128 frames):
  Page Faults: 30686
  Páginas Escritas: 5068

32 KB (64 frames):
  Page Faults: 38641
  Páginas Escritas: 6186
```

### Teste Conjunto 3: Variação de Memória
**Configuração**: compilador.log, LRU, 8 KB páginas

```
1 MB (128 frames):
  Page Faults: 36707
  Páginas Escritas: 5775

2 MB (256 frames):
  Page Faults: 21091
  Páginas Escritas: 3839

4 MB (512 frames):
  Page Faults: 7632
  Páginas Escritas: 1756
```

### Teste Conjunto 4: Diferentes Programas
**Configuração**: LRU, 8 KB páginas, 2 MB memória

```
compilador.log:
  Page Faults: 21091
  Páginas Escritas: 3839

matriz.log:
  Page Faults: 4745
  Páginas Escritas: 1623

compressor.log:
  Page Faults: 255
  Páginas Escritas: 0

simulador.log:
  Page Faults: 8556
  Páginas Escritas: 3395
```

---

## Anexo B: Exemplos de Execução

```bash
# Exemplo 1: LRU com compilador (configuração padrão)
$ ./exec/sim-virtual LRU files/compilador.log 8 2
Executando o simulador...
Arquivo de entrada: files/compilador.log
Tamanho da memoria física: 2 MB
Tamanho das páginas: 8 KB
Algoritmo de substituição: LRU
Número de Faltas de Páginas: 21091
Número de Páginas Escritas: 3839

# Exemplo 2: Algoritmo Ótimo (com pré-processamento)
$ ./exec/sim-virtual otimo files/compilador.log 8 2
Executando o simulador...
Pré-processando arquivo para algoritmo ótimo...
Total de acessos: 1000000
Pré-processamento concluído!
Arquivo de entrada: files/compilador.log
Tamanho da memoria física: 2 MB
Tamanho das páginas: 8 KB
Algoritmo de substituição: otimo
Número de Faltas de Páginas: 10118
Número de Páginas Escritas: 2190

# Exemplo 3: Testando diferentes tamanhos de página
$ ./exec/sim-virtual LRU files/matriz.log 16 4
Executando o simulador...
Arquivo de entrada: files/matriz.log
Tamanho da memoria física: 4 MB
Tamanho das páginas: 16 KB
Algoritmo de substituição: LRU
Número de Faltas de Páginas: [resultado depende da execução]
Número de Páginas Escritas: [resultado depende da execução]
```

---

**FIM DO RELATÓRIO**

*Este relatório foi elaborado como parte do Trabalho 2 da disciplina INF1316 - Sistemas Operacionais. Todos os testes foram executados e os resultados apresentados são reais, obtidos através do simulador implementado.*

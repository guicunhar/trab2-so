# Simulador de Memória Virtual — Trabalho 2 (Sistemas Operacionais)

Este projeto implementa um simulador de gerenciamento de memória virtual utilizando paginação.  
O programa recebe como entrada um arquivo de log contendo acessos à memória e executa o algoritmo de substituição escolhido (LRU, NRU ou Ótimo).

---

## 🚀 Como compilar

Na **raiz do projeto**, execute:
`make`

Isso irá gerar uma pasta chamada **exec/** contendo o binário:
`./exec/sim-virtual`

---

## ▶️ Como executar

O simulador exige **4 argumentos obrigatórios**, na seguinte ordem:
./exec/sim-virtual <algoritmo> <arquivo.log> <tamanho_pagina_kb> <memoria_mb>

Exemplo:
./exec/sim-virtual LRU files/compilador.log 8 2

---

## 📂 Estrutura do projeto

- **exec/** → onde o executável final é gerado  
- **src/** → arquivos `.c` e `.h` do simulador  
- **files/** → arquivos de log utilizados para teste  
- **Makefile** → script de compilação  

Os arquivos de log fornecidos são:

- `compilador`
- `compressor`
- `matriz`
- `simulador`

Eles devem ser descompactados dentro da pasta **files/**.

---

## 🔎 Validação dos argumentos

O programa já implementa:

- Verificação de número insuficiente de argumentos  
- Mensagem de uso correto do programa  
- Validação de tamanho de página  
  - valores permitidos: **8, 16, 32 KB**
- Validação da memória física
  - valores permitidos: **1, 2, 4 MB**
- Cálculo automático do *shift* (bits descartados) conforme o tamanho da página  
- Abertura e validação do arquivo `.log`
- Leitura do primeiro endereço lógico e operação para teste
- Conversão do endereço lógico para número de página

---

## 🧩 O que já foi implementado

O programa já contém:

- Tratamento completo dos argumentos de entrada
- Verificação do tamanho da página e memória
- Cálculo automático do deslocamento (shift)
- Leitura do arquivo de log
- Conversão de endereço lógico → número de página
- Impressão de endereço lido e página calculada para teste

---

## 🏗️ O que falta implementar

As próximas etapas obrigatórias do simulador são:

### 1️⃣ Criar a estrutura da **Tabela de Páginas**
- Cada página deve armazenar informações como: presente, frame, bits R/M etc.

### 2️⃣ Criar o **array de frames**
- Estruturas que representam cada quadro físico da memória
- Cada frame deve indicar se está livre, qual página contém, e metadados úteis

### 3️⃣ Fazer funcionar a parte **sem substituição**
- Carregar páginas enquanto houver frames livres  
- Atualizar tabela de páginas e frames  
- Contabilizar page hits e page faults iniciais

### 4️⃣ Implementar os algoritmos de substituição
Somente quando não houver mais frames livres:
- **LRU** — remove a página menos recentemente usada  
- **NRU** — usa classes baseadas nos bits R e M  
- **ÓTIMO** — remove a página cujo próximo uso é mais distante

---

## 📜 Observação

O código mostrado no projeto (main.c) foi usado apenas para testar leitura de argumentos e arquivos.  
A lógica real do simulador deve ser construída a partir dos passos acima.

---

## ✔️ Autor

Trabalho desenvolvido como parte da disciplina **Sistemas Operacionais — Gerência de Memória**.





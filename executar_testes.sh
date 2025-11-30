#!/bin/bash

# Script para executar todos os testes do simulador de memória virtual

echo "========================================="
echo "TESTES DO SIMULADOR DE MEMÓRIA VIRTUAL"
echo "========================================="
echo ""

RESULTADO="resultados_testes.txt"
echo "Resultados dos Testes - Simulador de Memória Virtual" > $RESULTADO
echo "Data: $(date)" >> $RESULTADO
echo "=========================================" >> $RESULTADO
echo "" >> $RESULTADO

ALGORITMOS=("LRU" "NRU" "otimo")
ARQUIVOS=("compilador.log" "matriz.log" "compressor.log" "simulador.log")
PAGINAS=(8 16 32)
MEMORIAS=(1 2 4)

TOTAL_TESTES=0
TESTE_ATUAL=0

for alg in "${ALGORITMOS[@]}"; do
    for arq in "${ARQUIVOS[@]}"; do
        for pag in "${PAGINAS[@]}"; do
            for mem in "${MEMORIAS[@]}"; do
                ((TOTAL_TESTES++))
            done
        done
    done
done

echo "Total de testes a executar: $TOTAL_TESTES"
echo ""

for alg in "${ALGORITMOS[@]}"; do
    for arq in "${ARQUIVOS[@]}"; do
        for pag in "${PAGINAS[@]}"; do
            for mem in "${MEMORIAS[@]}"; do
                ((TESTE_ATUAL++))

                echo "----------------------------------------"
                echo "Teste $TESTE_ATUAL/$TOTAL_TESTES"
                echo "Algoritmo: $alg | Arquivo: $arq | Página: ${pag}KB | Memória: ${mem}MB"
                echo "----------------------------------------"

                echo "" >> $RESULTADO
                echo "=== TESTE $TESTE_ATUAL ===" >> $RESULTADO
                echo "Algoritmo: $alg" >> $RESULTADO
                echo "Arquivo: $arq" >> $RESULTADO
                echo "Tamanho da Página: ${pag} KB" >> $RESULTADO
                echo "Tamanho da Memória: ${mem} MB" >> $RESULTADO
                echo "---" >> $RESULTADO

                ./exec/sim-virtual $alg files/$arq $pag $mem >> $RESULTADO 2>&1

                echo "" >> $RESULTADO
                echo ""
            done
        done
    done
done

echo ""
echo "========================================="
echo "TESTES CONCLUÍDOS!"
echo "========================================="
echo "Resultados salvos em: $RESULTADO"
echo ""
echo "Para ver os resultados:"
echo "  cat $RESULTADO"
echo ""

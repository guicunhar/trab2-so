#!/bin/bash

# Script para testes rápidos

echo "========================================="
echo "TESTES RÁPIDOS - SIMULADOR"
echo "========================================="
echo ""

RESULTADO="resultados_rapidos.txt"
echo "Resultados - Testes Rápidos" > $RESULTADO
echo "Data: $(date)" >> $RESULTADO
echo "" >> $RESULTADO

# Testa cada algoritmo com compilador.log, página 8KB, memória 2MB
ALGORITMOS=("LRU" "NRU" "otimo")

for alg in "${ALGORITMOS[@]}"; do
    echo "Testando $alg..."
    echo "=== $alg ===" >> $RESULTADO
    ./exec/sim-virtual $alg files/compilador.log 8 2 >> $RESULTADO 2>&1
    echo "" >> $RESULTADO
    echo ""
done

echo "Concluído! Veja: $RESULTADO"

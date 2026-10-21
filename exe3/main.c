#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include <stdio.h>

#include "data.h"

// Tamanho da janela da média móvel (constante nomeada, não número mágico)
#define MEDIA_JANELA 5

QueueHandle_t xQueueData;

// não mexer! Alimenta a fila com os dados do sinal
void data_task(void *p) {
    vTaskDelay(pdMS_TO_TICKS(400));

    int data_len = sizeof(sine_wave_four_cycles) / sizeof(sine_wave_four_cycles[0]);
    for (int i = 0; i < data_len; i++) {
        xQueueSend(xQueueData, &sine_wave_four_cycles[i], 1000000);
    }

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void process_task(void *p) {
    int data = 0;

    // Estado do filtro: variáveis LOCAIS da task (nada de global, Rule 4.4)
    // A janela começa com zeros: as 4 primeiras saídas já são impressas,
    // como se houvesse amostras 0 antes do início do sinal.
    int janela[MEDIA_JANELA] = {0};  // últimas 5 amostras (buffer circular)
    int indice = 0;                  // posição da amostra MAIS ANTIGA
    int soma = 0;                    // soma das amostras dentro da janela

    while (true) {
        if (xQueueReceive(xQueueData, &data, 100)) {
            // Média móvel na forma RECURSIVA: em vez de somar os 5 valores
            // toda vez, atualiza a soma: tira o mais antigo e põe o novo.
            soma -= janela[indice];   // descarta a amostra mais antiga
            janela[indice] = data;    // guarda a nova no lugar dela
            soma += data;             // inclui a nova na soma

            // Avança o índice circularmente: 0,1,2,3,4,0,1,...
            indice = (indice + 1) % MEDIA_JANELA;

            // Imprime a média a cada amostra (divisão inteira)
            printf("%d\n", soma / MEDIA_JANELA);

            // deixar esse delay!
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

int main() {
    stdio_init_all();

    xQueueData = xQueueCreate(64, sizeof(int));

    xTaskCreate(data_task, "Data task ", 4096, NULL, 1, NULL);
    xTaskCreate(process_task, "Process task", 4096, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}
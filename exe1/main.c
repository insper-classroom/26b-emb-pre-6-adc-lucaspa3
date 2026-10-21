#include <stdio.h>

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>

#include "pico/stdlib.h"

// Mutex: protege o recurso compartilhado (a saída serial / printf).
// Global porque as duas tasks precisam dele. Recurso do RTOS (Rule 4.4).
SemaphoreHandle_t xPrintfMutex;

void task_a(void *p) {
    while (true) {
        // Pega o mutex. portMAX_DELAY = espera o quanto for preciso.
        // Se a task B estiver com ele, a A dorme aqui até a B devolver.
        if (xSemaphoreTake(xPrintfMutex, portMAX_DELAY) == pdTRUE) {
            // SEÇÃO CRÍTICA: só uma task por vez executa este trecho
            printf("Task A: inicio da mensagem\n");
            vTaskDelay(pdMS_TO_TICKS(50));  // a B pode rodar, mas trava no Take
            printf("Task A: fim da mensagem\n");

            // Devolve o mutex: agora a B pode imprimir
            xSemaphoreGive(xPrintfMutex);
        }

        // Fora da seção crítica: espera sem segurar o mutex
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void task_b(void *p) {
    while (true) {
        if (xSemaphoreTake(xPrintfMutex, portMAX_DELAY) == pdTRUE) {
            printf("Task B: inicio da mensagem\n");
            vTaskDelay(pdMS_TO_TICKS(50));
            printf("Task B: fim da mensagem\n");

            xSemaphoreGive(xPrintfMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

int main() {
    stdio_init_all();

    // Cria o mutex ANTES das tasks. Diferente do semáforo binário,
    // o mutex nasce LIVRE: o primeiro Take já passa.
    xPrintfMutex = xSemaphoreCreateMutex();

    xTaskCreate(task_a, "Task A", 1024, NULL, 1, NULL);
    xTaskCreate(task_b, "Task B", 1024, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true) {
    }
}
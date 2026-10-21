#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include <stdio.h>

#include "hardware/gpio.h"
#include "hardware/adc.h"

void adc_task(void *p) {
    adc_init();
    adc_gpio_init(27);   // potenciômetro 1 (canal 1)
    adc_gpio_init(26);   // potenciômetro 2 (canal 0)

    // 12-bit conversion, assume max value == ADC_VREF == 3.3 V
    const float conversion_factor = 3.3f / (1 << 12);

    uint16_t result;
    while (1) {
        adc_select_input(1); // Select ADC input 1 (GPIO27)
        result = adc_read();
        printf("voltage 1: %f V\n", result * conversion_factor);

        // Mesmo padrão: selecionar o canal e depois ler.
        // GP26 corresponde ao canal 0 do MUX (o número do pino não é o do canal).
        adc_select_input(0); // Select ADC input 0 (GPIO26)
        result = adc_read();
        printf("voltage 2: %f V\n", result * conversion_factor);

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

int main() {
    stdio_init_all();
    printf("Start RTOS \n");
    adc_init();

    xTaskCreate(adc_task, "ADC_Task", 4095, NULL, 1, NULL);
    vTaskStartScheduler();

    while (true) {
    }
}
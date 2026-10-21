/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

const int PIN_LED_B = 4;

// >>> CONFIRMAR no diagram.json: pino do potenciômetro e canal do MUX <<<
const int PIN_POT = 28;     // GP28
const int ADC_CANAL = 2;    // GP26 -> 0, GP27 -> 1, GP28 -> 2

const float conversion_factor = 3.3f / (1 << 12);

// Limites das zonas (V)
const float LIMITE_ZONA_1 = 1.0f;
const float LIMITE_ZONA_2 = 2.0f;

// Períodos de pisca por zona (ms). Enunciado: 300 / 500 (comentário antigo: 150 / 400)
const int PERIODO_ZONA_1_MS = 300;
const int PERIODO_ZONA_2_MS = 500;

// Flag da interrupção do timer. Global acessada na IRQ -> volatile (regra do checker)
volatile int flag_timer = 0;

// CALLBACK DO TIMER (contexto de interrupção): curto, só anota o evento
bool timer_callback(struct repeating_timer *t) {
    flag_timer = 1;
    return true;   // true = continua repetindo
}

/**
 * 0..1.0V: Desligado
 * 1..2.0V: 300 ms
 * 2..3.3V: 500 ms
 */
int main() {
    stdio_init_all();

    // LED
    gpio_init(PIN_LED_B);
    gpio_set_dir(PIN_LED_B, GPIO_OUT);
    gpio_put(PIN_LED_B, 0);

    // ADC
    adc_init();
    adc_gpio_init(PIN_POT);
    adc_select_input(ADC_CANAL);

    // Estado local do main (não acessado por IRQ -> sem volatile)
    struct repeating_timer timer;
    int timer_ativo = 0;
    int zona_atual = 0;   // começa na zona 0: LED apagado
    int led = 0;          // estado do LED (sem gpio_get)

    while (1) {
        // 1) Lê o ADC e descobre a zona
        float tensao = adc_read() * conversion_factor;

        int zona;
        if (tensao < LIMITE_ZONA_1) {
            zona = 0;
        } else if (tensao < LIMITE_ZONA_2) {
            zona = 1;
        } else {
            zona = 2;
        }

        // 2) Só mexe no timer quando a zona MUDA
        if (zona != zona_atual) {
            zona_atual = zona;

            // Para o timer antigo, se houver
            if (timer_ativo) {
                cancel_repeating_timer(&timer);
                timer_ativo = 0;
            }

            if (zona == 0) {
                // Zona 0: LED sempre apagado
                led = 0;
                gpio_put(PIN_LED_B, 0);
                flag_timer = 0;
            } else {
                // Zonas 1 e 2: novo timer com o período da zona
                int periodo = (zona == 1) ? PERIODO_ZONA_1_MS : PERIODO_ZONA_2_MS;
                add_repeating_timer_ms(periodo, timer_callback, NULL, &timer);
                timer_ativo = 1;
            }
        }

        // 3) A cada estouro do timer, inverte o LED (trabalho fora da IRQ)
        if (flag_timer) {
            flag_timer = 0;
            led = !led;
            gpio_put(PIN_LED_B, led);
        }
    }
}
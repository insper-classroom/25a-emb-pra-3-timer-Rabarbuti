/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"


const int TRIGGER = 14;
const int ECHO = 15;

volatile uint32_t t0;
volatile uint32_t tf;
volatile int erro = 0;

void echo_callback(uint gpio, uint32_t events){
    if (gpio_get(ECHO)) {
        t0 = to_us_since_boot(get_absolute_time());
    } else {
        tf = to_us_since_boot(get_absolute_time());
    }
}

int64_t alarm_callback(alarm_id_t id, void *user_data) {
    erro = 1;
    return 0;
}

int main() {
    stdio_init_all();

    gpio_init(ECHO);
    gpio_set_dir(ECHO, GPIO_IN);

    gpio_init(TRIGGER);
    gpio_set_dir(TRIGGER, GPIO_OUT);

    gpio_set_irq_enabled_with_callback(ECHO, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &echo_callback);

    datetime_t dt = {
            .year = 2025,
            .month = 3,
            .day = 17,
            .hour = 10,
            .min = 30,
            .sec = 0
    };

    rtc_init();
    rtc_set_datetime(&dt);

    while (true) {
        char start_char;
        printf("Aperte 's'\n");
        scanf("%c", &start_char);
        int stop_char;

        while(start_char == 's' && stop_char != 'p'){
            stop_char = getchar_timeout_us(1000);
            char datetime_buf[256];
            char *datetime_str = &datetime_buf[0];

            datetime_to_str(datetime_str, sizeof(datetime_buf), &dt);
            sleep_ms(300);
            gpio_put(TRIGGER, 1);
            sleep_us(10);
            gpio_put(TRIGGER, 0);

            alarm_id_t alarm = add_alarm_in_ms(500, alarm_callback, NULL, false);

            while( erro == 0 && tf == 0){}//VAI ESPERAR O ALARME OU O ECHO

            rtc_get_datetime(&dt);

            if(erro == 1){ // se não houve erro na leitura alarme desativa
                printf("%s - FALHA NA LEITURA \n", datetime_str);
            } else {
                cancel_alarm(alarm);
                double dist = (tf - t0) * 0.0343 / 2; // calculo da distancia do sensor
                printf("%s - %lf cm\n", datetime_str, dist);
            }
            erro = 0; //resetar as flags para evitar bugs
            tf = 0;
            stop_char = 0;
        }

    }
}

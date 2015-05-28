#if 0
#include <stdbool.h>
#include <stdint.h>
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "nrf_drv_gpiote.h"
#include "app_error.h"
#include "nrf_drv_clock.h"
#include "app_uart.h"
#include "debug.h"


#define UART_TX_BUF_SIZE 128u /**< UART Tx buffer size. */
#define UART_RX_BUF_SIZE 1u   /**< UART Rx buffer size. */

void uart_error_handle(app_uart_evt_t * p_event)
{
   // No implementation needed.
}




const uint8_t leds_list[LEDS_NUMBER] = LEDS_LIST;
static uint8_t selected_led_num = 0;

void button_press_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    nrf_delay_ms(20);


    if(!nrf_gpio_pin_read(pin))
    {
        printf("pressed button");
        nrf_drv_gpiote_out_clear(leds_list[selected_led_num]);
        nrf_delay_ms(500);
        nrf_drv_gpiote_out_set(leds_list[selected_led_num]);
        if(selected_led_num++ >= LEDS_NUMBER-1)
        {
            selected_led_num = 0;
        }
    }
}



void ButtonInit(void)
{
    ret_code_t err_code;

    err_code = nrf_drv_gpiote_init();
    APP_ERROR_CHECK(err_code);

    //初始化LED端口为高电平，即灭的状态
    nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_SIMPLE(true);

    for(int i = 0; i < LEDS_NUMBER; i++)
    {
        err_code = nrf_drv_gpiote_out_init(leds_list[i], &out_config);
        APP_ERROR_CHECK(err_code);
    }

    //按键设置为上拉中断模式
    nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
    in_config.pull = NRF_GPIO_PIN_PULLUP;

    err_code = nrf_drv_gpiote_in_init(USER_BUTTON, &in_config, button_press_handler);
    APP_ERROR_CHECK(err_code);

    nrf_drv_gpiote_in_event_enable(USER_BUTTON, true);
}


void ReadOutProtect()
{
    NRF_UICR->RBPCONF = 0x0000;
}


uint32_t GetFirmID()
{
    return NRF_UICR->FWID;
}







/**
 * @brief Function for application main entry.
 */
int main(void)
{
    uint32_t err_code;

    //配置串口
    const app_uart_comm_params_t comm_params =
    {
        RX_PIN_NUMBER,
        TX_PIN_NUMBER,
        RTS_PIN_NUMBER,
        CTS_PIN_NUMBER,
        APP_UART_FLOW_CONTROL_DISABLED,
        false,
        UART_BAUDRATE_BAUDRATE_Baud9600
    };

    APP_UART_FIFO_INIT(&comm_params,
                         UART_RX_BUF_SIZE,
                         UART_TX_BUF_SIZE,
                         uart_error_handle,
                         APP_IRQ_PRIORITY_LOW,
                         err_code);

    APP_ERROR_CHECK(err_code);
		printf("hello");

    // 跑马灯测试
    // // Configure LED-pins as outputs.
    // LEDS_CONFIGURE(LEDS_MASK);
    // // Toggle LEDs.
    // while (true)
    // {
    //     for (int i = 0; i < LEDS_NUMBER; i++)
    //     {
    //         LEDS_INVERT(1 << leds_list[i]);
    //         nrf_delay_ms(500);
    //     }
    // }

    //配置晶振为16M或者32M
    //err_code = nrf_drv_clock_init(NULL);
    //APP_ERROR_CHECK(err_code);

    //按键配置为中断模式，按键按下后调试灯会交替闪烁
    ButtonInit();
		

    while(true)
    {
        ;
    }
}


/** @} */
#endif


/* Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

/** @file
 * @defgroup uart_example_main main.c
 * @{
 * @ingroup uart_example
 * @brief UART Example Application main file.
 *
 * This file contains the source code for a sample application using UART.
 * 
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "app_uart.h"
#include "app_error.h"
#include "nrf_delay.h"
#include "nrf.h"
#include "boards.h"

//#define ENABLE_LOOPBACK_TEST  /**< if defined, then this example will be a loopback test, which means that TX should be connected to RX to get data loopback. */

#define MAX_TEST_DATA_BYTES     (15U)                /**< max number of test bytes to be used for tx and rx. */
#define UART_TX_BUF_SIZE 256                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE 1                           /**< UART RX buffer size. */

void uart_error_handle(app_uart_evt_t * p_event)
{
    if (p_event->evt_type == APP_UART_COMMUNICATION_ERROR)
    {
        APP_ERROR_HANDLER(p_event->data.error_communication);
    }
    else if (p_event->evt_type == APP_UART_FIFO_ERROR)
    {
        APP_ERROR_HANDLER(p_event->data.error_code);
    }
}



#ifdef ENABLE_LOOPBACK_TEST
/** @brief Function for setting the @ref ERROR_PIN high, and then enter an infinite loop.
 */
static void show_error(void)
{
    
    LEDS_ON(LEDS_MASK);
    while(true)
    {
        // Do nothing.
    }
}


/** @brief Function for testing UART loop back. 
 *  @details Transmitts one character at a time to check if the data received from the loopback is same as the transmitted data.
 *  @note  @ref TX_PIN_NUMBER must be connected to @ref RX_PIN_NUMBER)
 */
static void uart_loopback_test()
{
    uint8_t * tx_data = (uint8_t *)("\n\rLOOPBACK_TEST\n\r");
    uint8_t   rx_data;

    // Start sending one byte and see if you get the same
    for (uint32_t i = 0; i < MAX_TEST_DATA_BYTES; i++)
    {
        uint32_t err_code;
        while(app_uart_put(tx_data[i]) != NRF_SUCCESS);

        nrf_delay_ms(10);
        err_code = app_uart_get(&rx_data);

        if ((rx_data != tx_data[i]) || (err_code != NRF_SUCCESS))
        {
            show_error();
        }
    }
    return;
}


#endif


/**
 * @brief Function for main application entry.
 */
int main(void)
{
    LEDS_CONFIGURE(LEDS_MASK);
    LEDS_OFF(LEDS_MASK);
    uint32_t err_code;
    const app_uart_comm_params_t comm_params =
      {
          RX_PIN_NUMBER,
          TX_PIN_NUMBER,
          RTS_PIN_NUMBER,
          CTS_PIN_NUMBER,
          APP_UART_FLOW_CONTROL_DISABLED,
          false,
          UART_BAUDRATE_BAUDRATE_Baud38400
      };

    APP_UART_FIFO_INIT(&comm_params,
                         UART_RX_BUF_SIZE,
                         UART_TX_BUF_SIZE,
                         uart_error_handle,
                         APP_IRQ_PRIORITY_LOW,
                         err_code);

    APP_ERROR_CHECK(err_code);

#ifndef ENABLE_LOOPBACK_TEST
    printf("\n\rStart: \n\r");

    while (true)
    {
        uint8_t cr;
        while(app_uart_get(&cr) != NRF_SUCCESS);
        while(app_uart_put(cr) != NRF_SUCCESS);

        if (cr == 'q' || cr == 'Q')
        {
            printf(" \n\rExit!\n\r");

            while (true)
            {
                // Do nothing.
            }
        }
    }
#else

    // This part of the example is just for testing the loopback .
    while (true)
    {
        uart_loopback_test();
    }
#endif
}


/** @} */

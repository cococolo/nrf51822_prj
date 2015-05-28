#include <stdbool.h>
#include <stdint.h>
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "nrf_drv_gpiote.h"
#include "app_error.h"

const uint8_t leds_list[LEDS_NUMBER] = LEDS_LIST;


void button_press_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    nrf_delay_ms(20);

    if(!nrf_gpio_pin_read(pin))
    {
        nrf_drv_gpiote_out_toggle(leds_list[0]);
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








/**
 * @brief Function for application main entry.
 */
int main(void)
{
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

    ButtonInit();

    while(true)
    {
        ;
    }
}


/** @} */

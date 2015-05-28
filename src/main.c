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
#include "radio_config.h"


//#define TX 1



#define UART_TX_BUF_SIZE 128u /**< UART Tx buffer size. */
#define UART_RX_BUF_SIZE 1u   /**< UART Rx buffer size. */


#define AK2_LED0 18
#define AK2_LED1 19
#define AK2_LED2 20
#define AK2_LED3 21
#define AK2_LED4 22
#define AK2_LED0_MASK (1<<AK2_LED0)
#define AK2_LED1_MASK (1<<AK2_LED1)
#define AK2_LED2_MASK (1<<AK2_LED2)
#define AK2_LED3_MASK (1<<AK2_LED3)
#define AK2_LED4_MASK (1<<AK2_LED4)
#define AK2_LEDS_MASK      (AK2_LED0_MASK | AK2_LED1_MASK | AK2_LED2_MASK | \
                            AK2_LED3_MASK | AK2_LED4_MASK)



void send_packet();



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
        debug("pressed button and send rf msg");
        send_packet();
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


//无线测试
void clock_initialization()
{
    /* Start 16 MHz crystal oscillator */
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART    = 1;

    /* Wait for the external oscillator to start up */
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0)
    {
        // Do nothing.
    }

    /* Start low frequency crystal oscillator for app_timer(used by bsp)*/
    NRF_CLOCK->LFCLKSRC            = (CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos);
    NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_LFCLKSTART    = 1;

    while (NRF_CLOCK->EVENTS_LFCLKSTARTED == 0)
    {
        // Do nothing.
    }
}

static uint32_t                   packet;

/**@brief Function for sending packet.
 */
void send_packet()
{

    // Set payload pointer
    NRF_RADIO->PACKETPTR = (uint32_t)&packet;

    // send the packet:
    NRF_RADIO->EVENTS_READY = 0U;
    NRF_RADIO->TASKS_TXEN   = 1;

    while (NRF_RADIO->EVENTS_READY == 0U)
    {
        // wait
    }
    NRF_RADIO->EVENTS_END  = 0U;
    NRF_RADIO->TASKS_START = 1U;

    while (NRF_RADIO->EVENTS_END == 0U)
    {
        // wait
    }

    // uint32_t err_code = bsp_indication_text_set(BSP_INDICATE_SENT_OK, "The packet was sent\n\r");
    // APP_ERROR_CHECK(err_code);

    NRF_RADIO->EVENTS_DISABLED = 0U;
    // Disable radio
    NRF_RADIO->TASKS_DISABLE = 1U;

    while (NRF_RADIO->EVENTS_DISABLED == 0U)
    {
        // wait
    }
}




/**@brief Function for reading packet.
 */
uint32_t read_packet()
{
    uint32_t result = 0;

    NRF_RADIO->PACKETPTR = (uint32_t)&packet;

    NRF_RADIO->EVENTS_READY = 0U;
    // Enable radio and wait for ready
    NRF_RADIO->TASKS_RXEN = 1U;

    while (NRF_RADIO->EVENTS_READY == 0U)
    {
        // wait
    }
    NRF_RADIO->EVENTS_END = 0U;
    // Start listening and wait for address received event
    NRF_RADIO->TASKS_START = 1U;

    // Wait for end of packet or buttons state changed
    while (NRF_RADIO->EVENTS_END == 0U)
    {
        // wait
    }

    if (NRF_RADIO->CRCSTATUS == 1U)
    {
        result = packet;
    }
    NRF_RADIO->EVENTS_DISABLED = 0U;
    // Disable radio
    NRF_RADIO->TASKS_DISABLE = 1U;

    while (NRF_RADIO->EVENTS_DISABLED == 0U)
    {
        // wait
    }
    return result;
}





/**
 * @brief Function for application main entry.
 */
int main(void)
{
    uint32_t err_code;

    clock_initialization();

    //配置串口
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
    printf("uart init succeed!\n");

#ifndef TX
    LEDS_CONFIGURE(AK2_LEDS_MASK);
    LEDS_ON(AK2_LEDS_MASK);
#else
    
#endif

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
    // err_code = nrf_drv_clock_init(NULL);
    // APP_ERROR_CHECK(err_code);

    
    //按键配置为中断模式，按键按下后调试灯会交替闪烁
    ButtonInit();
    radio_configure();

    while(true)
    {
#ifdef TX
        //do nothing
#else
        uint32_t received = read_packet();
        printf("The contents of the package is %u\n\r", (unsigned int)received);
        LEDS_OFF(AK2_LEDS_MASK);
        nrf_delay_ms(500);
        LEDS_ON(AK2_LEDS_MASK);

#endif
    }
}


/** @} */

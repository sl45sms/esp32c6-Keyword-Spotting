#include "light.h"
#include "led_strip.h"

void light(bool on, bool init) {
        static led_strip_handle_t led_strip;
       //light the led at gpio8 to indicate recording
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = GPIO_NUM_8,
        .max_leds = 1, // at least one LED on board
    };
    led_strip_spi_config_t spi_config = {
        .spi_bus = SPI2_HOST,
        .flags = {
            .with_dma = true,
        },
    };
    //check if led_strip is already initialized
    if (init == true && led_strip == NULL)
    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip)); 
    if (!on) ESP_ERROR_CHECK(led_strip_clear(led_strip));else
    {
        ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, 16, 16, 16));
        ESP_ERROR_CHECK(led_strip_refresh(led_strip));
    }
    //delete spi device
    //ESP_ERROR_CHECK(led_strip_delete(&led_strip));

 }
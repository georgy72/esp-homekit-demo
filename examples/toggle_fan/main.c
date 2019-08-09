/*
 * In order to flash the Fan basic you will have to
 * have a 3,3v (logic level) FTDI adapter.
 *
 * To flash this example connect 3,3v, TX, RX, GND
 * in this order, beginning in the (square) pin header
 * next to the button.
 * Next hold down the button and connect the FTDI adapter
 * to your computer. The Fan is now in flash mode and
 * you can flash the custom firmware.
 *
 * WARNING: Do not connect the Fan to AC while it's
 * connected to the FTDI adapter! This may fry your
 * computer and Fan.
 *
 */

#include <stdio.h>
#include <string.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <espressif/esp_common.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <wifi_config.h>

#include "contact_sensor.h"
#include "button.h"

#define NO_CONNECTION_WATCHDOG_TIMEOUT 120000

const int led_station_read_gpio = 14;
const int one_hours_button_write_gpio = 12;
const int big_button_read_gpio = 0;
const int big_button_write_gpio = 0;
const int led_on_board_gpio = 2;

bool is_connected_to_wifi = false;
void switch_on_callback(homekit_characteristic_t *_ch, homekit_value_t on, void *context);
void button_callback(uint8_t gpio, button_event_t event);

void led_write(bool on) {
    gpio_write(led_on_board_gpio, on ? 0 : 1);
}

bool led_read() {
    return gpio_read(led_on_board_gpio);
}

void led_blink(int times) {
    bool led_value = led_read();
    for (int i=0; i<times; i++) {
        led_write(true);
        vTaskDelay(100 / portTICK_PERIOD_MS);
        led_write(false);
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    led_write(led_value);
}

void on_fan(){
    gpio_write(one_hours_button_write_gpio, true);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_write(one_hours_button_write_gpio, false);
}

void off_fan(){
     & !switch_on.value.bool_value
    gpio_write(big_button_write_gpio, true);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_write(big_button_write_gpio, false);
}

void toggle_fan(bool on) {
    if (on){
        on_fan();
    } else {
        off_fan();
    }
}

void reset_configuration_task() {

    led_blink(3);
    
    printf("Resetting HomeKit Config\n");
    
    homekit_server_reset();
    
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    printf("Resetting Wifi Config\n");
    
    wifi_config_reset();
    
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    printf("Restarting\n");
    
    sdk_system_restart();
    
    vTaskDelete(NULL);
}

void reset_configuration() {
    printf("Resetting Fan configuration\n");
    xTaskCreate(reset_configuration_task, "Reset configuration", 256, NULL, 2, NULL);
}

homekit_characteristic_t switch_on = HOMEKIT_CHARACTERISTIC_(
    ON, false, .callback=HOMEKIT_CHARACTERISTIC_CALLBACK(switch_on_callback)
);

void gpio_init() {
    gpio_enable(led_on_board_gpio, GPIO_OUTPUT);
    led_write(false);
    
    gpio_enable(one_hours_button_write_gpio, GPIO_OUTPUT);
    gpio_enable(led_station_read_gpio, GPIO_INPUT);
    gpio_enable(big_button_read_gpio, GPIO_INPUT);
    gpio_enable(big_button_write_gpio, GPIO_OUTPUT);
}

void gpio_update(bool value) {
    switch_on.value.bool_value = read_gpio(led_station_read_gpio);
    printf("Station fan Value: %d\n", switch_on.value.bool_value);
    homekit_characteristic_notify(&switch_on, switch_on.value);
}

void switch_on_callback(homekit_characteristic_t *_ch, homekit_value_t on, void *context) {
    toggle_fan(switch_on.value.bool_value);
}

void button_callback(uint8_t gpio, button_event_t event) {
    switch (event) {
        case button_event_single_press:
            break;
        case button_event_long_press:
            reset_configuration();
            break;
        default:
            printf("Unknown button event: %d\n", event);
    }
}

void contact_sensor_callback(uint8_t gpio, contact_sensor_state_t state) {
    
    if (state){
        printf("Toggling on FAN\n");
    } else {
        printf("Toggling off FAN\n");
    }

    switch_on.value.bool_value = state;
    led_write(state);

    homekit_characteristic_notify(&switch_on, switch_on.value);
}

void switch_identify_task(void *_args) {
    // We identify the Fan by Flashing it's LED.
    for (int i=0; i<3; i++) {
        led_blink(2);
    }

    vTaskDelete(NULL);
}

void switch_identify(homekit_value_t _value) {
    printf("Switch identify\n");
    xTaskCreate(switch_identify_task, "Switch identify", 128, NULL, 2, NULL);
}

void wifi_connection_watchdog_task(void *_args) {
    vTaskDelay(NO_CONNECTION_WATCHDOG_TIMEOUT / portTICK_PERIOD_MS);

    if (is_connected_to_wifi == false) {
        led_blink(3);
        printf("No Wifi Connection, Restarting\n");
        sdk_system_restart();
    }
    
    vTaskDelete(NULL);
}

void create_wifi_connection_watchdog() {
    printf("Wifi connection watchdog\n");
    xTaskCreate(wifi_connection_watchdog_task, "Wifi Connection Watchdog", 128, NULL, 3, NULL);
}

homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, "Fan Switch");

homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_switch, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            &name,
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Fan"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "037A2BABF19D"),
            HOMEKIT_CHARACTERISTIC(MODEL, "Basic"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0.0"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, switch_identify),
            NULL
        }),
        HOMEKIT_SERVICE(SWITCH, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Fan Switch"),
            &switch_on,
            NULL
        }),
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "333-22-333"
};

void on_wifi_ready() {
    is_connected_to_wifi = true;
    homekit_server_init(&config);
}

void create_accessory_name() {
    uint8_t macaddr[6];
    sdk_wifi_get_macaddr(STATION_IF, macaddr);
    
    int name_len = snprintf(NULL, 0, "Fan Switch-%02X%02X%02X",
                            macaddr[3], macaddr[4], macaddr[5]);
    char *name_value = malloc(name_len+1);
    snprintf(name_value, name_len+1, "Fan Switch-%02X%02X%02X",
             macaddr[3], macaddr[4], macaddr[5]);
    
    name.value = HOMEKIT_STRING(name_value);
}

void user_init(void) {
    uart_set_baud(0, 115200);

    create_accessory_name();
    
    wifi_config_init("Fan-switch", NULL, on_wifi_ready);
    gpio_init();

    if (button_create(big_button_read_gpio, 0, 30000, button_callback)) {
        printf("Failed to initialize button\n");
    }
    if (contact_sensor_create(led_station_read_gpio, contact_sensor_callback)) {
        printf("Failed to initialize led_station_read_gpio\n");
    }

    create_wifi_connection_watchdog();

    gpio_update();
}

#include <stdio.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "say_hello.h"

// Settings
static const uint32_t sleep_time_ms = 1000;

// Tag for log messages
static const char *TAG = "kconfig_demo";

static void LogTask(void *arg)
{
    // Superloop
    while (1) 
    {
        // Log messages
        printf("Log messages:\n");
        ESP_LOGE(TAG, "Error");
        ESP_LOGW(TAG, "Warning");
        ESP_LOGI(TAG, "Info");
        // ESP_LOGD(TAG, "Debug");
        // ESP_LOGV(TAG, "Verbose");

        // Say hello
#ifdef CONFIG_SAY_HELLO
        say_hello();
#endif

        // Delay
        vTaskDelay(sleep_time_ms / portTICK_PERIOD_MS); 
    }
}


void app_main(void)
{
    xTaskCreate(LogTask, "LogTask", 2048, NULL, 5, NULL);
}
/*
 * ESP32-S3-WROOM-1 Built-in RGB LED Gradient Effect
 *
 * Replicates the factory "rainbow breathing" demo seen on fresh ESP32-S3 boards.
 * The onboard WS2812 RGB LED is wired to GPIO 48 and driven via the RMT peripheral.
 *
 * ESP-IDF >= 5.0 recommended (uses led_strip component).
 */

#include <stdio.h>
#include "esp_log.h"
#include "esp_err.h"
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"

static const char *TAG = "rgb_gradient";

/* ── Hardware config ─────────────────────────────────────── */
#define LED_GPIO        48          /* GPIO for the onboard WS2812 */
#define LED_RMT_CHANNEL 0
#define NUM_LEDS        1

/* ── Animation tuning ────────────────────────────────────── */
#define HUE_STEP_DEG    0.5f       /* degrees of hue advanced per tick  */
#define BREATH_SPEED    0.02f      /* radians advanced per tick          */
#define MAX_BRIGHTNESS  220        /* 0-255 ceiling (WS2812 is very bright) */
#define MIN_BRIGHTNESS  8          /* floor so the LED never fully dies  */
#define TICK_MS         10         /* loop period in milliseconds        */

/* ── Helpers ─────────────────────────────────────────────── */

/**
 * Convert HSV to RGB.
 * h: 0-360, s: 0-1, v: 0-1
 * Outputs r/g/b in range 0-255.
 */
static void hsv_to_rgb(float h, float s, float v,
                        uint8_t *r, uint8_t *g, uint8_t *b)
{
    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    float r1, g1, b1;

    if      (h < 60)  { r1 = c; g1 = x; b1 = 0; }
    else if (h < 120) { r1 = x; g1 = c; b1 = 0; }
    else if (h < 180) { r1 = 0; g1 = c; b1 = x; }
    else if (h < 240) { r1 = 0; g1 = x; b1 = c; }
    else if (h < 300) { r1 = x; g1 = 0; b1 = c; }
    else              { r1 = c; g1 = 0; b1 = x; }

    *r = (uint8_t)((r1 + m) * 255.0f);
    *g = (uint8_t)((g1 + m) * 255.0f);
    *b = (uint8_t)((b1 + m) * 255.0f);
}

/* ── Main task ───────────────────────────────────────────── */

static void LightingTask(void *arg)
{
    /* Configure the LED strip */
    led_strip_config_t strip_cfg = {
        .strip_gpio_num   = LED_GPIO,
        .max_leds         = NUM_LEDS,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,   /* WS2812 is GRB */
        .led_model        = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };

    led_strip_rmt_config_t rmt_cfg = {
        .clk_src        = RMT_CLK_SRC_DEFAULT,
        .resolution_hz  = 10 * 1000 * 1000,        /* 10 MHz */
        .flags.with_dma = false,
    };

    led_strip_handle_t strip;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_cfg, &rmt_cfg, &strip));
    ESP_ERROR_CHECK(led_strip_clear(strip));

    // ESP_LOGI(TAG, "RGB gradient started on GPIO %d", LED_GPIO);

    float hue        = 0.0f;   /* 0-360 degrees             */
    float breath_phi = 0.0f;   /* sine-wave phase for breathing */

    while (1) 
    {
        /* Breathing envelope: smooth sine wave, always positive */
        float breath = (sinf(breath_phi) + 1.0f) / 2.0f;   /* 0.0 – 1.0 */
        float brightness = MIN_BRIGHTNESS
                         + breath * (MAX_BRIGHTNESS - MIN_BRIGHTNESS);
        float v = brightness / 255.0f;

        /* Full saturation gives vivid rainbow colours */
        uint8_t r, g, b;
        hsv_to_rgb(hue, 1.0f, v, &r, &g, &b);

        ESP_ERROR_CHECK(led_strip_set_pixel(strip, 0, r, g, b));
        ESP_ERROR_CHECK(led_strip_refresh(strip));

        /* Advance state */
        hue += HUE_STEP_DEG;
        if (hue >= 360.0f) hue -= 360.0f;

        breath_phi += BREATH_SPEED;
        if (breath_phi >= 2.0f * (float)M_PI) breath_phi -= 2.0f * (float)M_PI;

        vTaskDelay(pdMS_TO_TICKS(TICK_MS));
    }
}

static void LogTask(void *arg)
{
    static const uint32_t sleep_time_ms = 1000;

    while (1) {
        // Log messages
        ESP_LOGE(TAG, "Error");
        ESP_LOGW(TAG, "Warning");
        ESP_LOGI(TAG, "Info");
        vTaskDelay(sleep_time_ms / portTICK_PERIOD_MS); 
    }
}

void app_main(void)
{
    xTaskCreatePinnedToCore(LightingTask,
                "rgb_gradient",
                4096,
                NULL,
                5,
                NULL,
                0);

    xTaskCreatePinnedToCore(LogTask,
                "log_task",
                2048,
                NULL,
                5,
                NULL,
                1);
}

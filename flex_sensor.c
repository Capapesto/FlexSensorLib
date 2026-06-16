/*
  ╔══════════════════════════════════════════════════════════╗
  ║              RAND — flex_sensor.c                       ║
  ╚══════════════════════════════════════════════════════════╝
 
  Fórmulas usadas:
 
    Tensão no pino ADC (divisor de tensão):
      V_adc = VCC × R_flex / (R_div + R_flex)
 
    Resistência do flex a partir do raw ADC:
      V_adc  = raw × V_ref / ADC_MAX
      R_flex = R_div × V_adc / (VCC - V_adc)
 
    Curvatura normalizada (0.0 plano … 1.0 dobrado):
      curvature = (R_flex - R_flat) / (R_bent - R_flat)
      clamped   = max(0.0, min(1.0, curvature))
 */

#include "flex_sensor.h"

#include <math.h>
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"

static const char *TAG = "FLEX";

// ── Handles do ADC ─────────────────────────────────────── 
static adc_oneshot_unit_handle_t s_adc_handle;

// ── Canais por índice de sensor ───────────────── 
static const adc_channel_t s_channels[FLEX_SENSOR_COUNT] = {
    ADC_CHANNEL_0,   // GPIO 0 → Flex 0 
    ADC_CHANNEL_1,   // GPIO 1 → Flex 1 
};

// ── Parâmetros ajustáveis em tempo de execução ─────────── 
static float s_r_div       = FLEX_R_DIV_OHM;
static float s_r_flat      = FLEX_R_FLAT_OHM;
static float s_r_bent      = FLEX_R_BENT_OHM;
static float s_threshold   = FLEX_BEND_THRESHOLD;


/* ════════════════════════════════════════════════════════════
   CONVERSÃO 
   ════════════════════════════════════════════════════════════ */

/*
  Converte raw ADC → resistência do flex sensor (ohms).
 
  Evita divisão por zero quando raw == ADC_MAX (tensão = VCC,
  o que indica curto ou sensor desconectado).
 */
static float raw_to_resistance(uint16_t raw)
{
    if (raw >= FLEX_ADC_MAX) return s_r_bent * 10.0f; /* valor sentinela */

    float v_adc = (float)raw * (float)FLEX_ADC_VREF_MV / (float)FLEX_ADC_MAX;
    float v_vcc = (float)FLEX_VCC_MV;

    //             R_flex = R_div × V_adc / (V_vcc - V_adc) 
    float denom = v_vcc - v_adc;
    if (fabsf(denom) < 1.0f) return s_r_bent * 10.0f;

    return s_r_div * v_adc / denom;
}

/*
 Converte resistência → curvatura normalizada
 */
static float resistance_to_curvature(float r_ohm)
{
    float range = s_r_bent - s_r_flat;
    if (fabsf(range) < 1.0f) return 0.0f;

    float c = (r_ohm - s_r_flat) / range;

    // Garante que fique entre 0.0 e 1.0 
    if (c < 0.0f) c = 0.0f;
    if (c > 1.0f) c = 1.0f;

    return c;
}


/* ════════════════════════════════════════════════════════════
   API PÚBLICA
   ════════════════════════════════════════════════════════════ */

esp_err_t flex_sensor_init(void)
{
    /* Configura a unidade ADC1 */
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id  = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    esp_err_t err = adc_oneshot_new_unit(&unit_cfg, &s_adc_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao criar unidade ADC: %s", esp_err_to_name(err));
        return err;
    }

    /* Configura cada canal */
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = ADC_ATTEN_DB_11,   // até ~3.1 V
        .bitwidth = ADC_BITWIDTH_12,   // 12 bits → 0 a 4095           
    };

    for (int i = 0; i < FLEX_SENSOR_COUNT; i++) {
        err = adc_oneshot_config_channel(s_adc_handle, s_channels[i], &chan_cfg);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Falha ao configurar canal %d: %s", i, esp_err_to_name(err));
            return err;
        }
    }

    ESP_LOGI(TAG, "Flex sensors inicializados. R_div=%.0f R_flat=%.0f R_bent=%.0f threshold=%.2f",
             s_r_div, s_r_flat, s_r_bent, s_threshold);

    return ESP_OK;
}

esp_err_t flex_sensor_read(uint8_t index, flex_reading_t *out)
{
    if (index >= FLEX_SENSOR_COUNT || !out) return ESP_ERR_INVALID_ARG;

    int raw = 0;
    esp_err_t err = adc_oneshot_read(s_adc_handle, s_channels[index], &raw);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Erro ao ler flex %u: %s", index, esp_err_to_name(err));
        return err;
    }

    out->raw        = (uint16_t)raw;
    out->resistance = raw_to_resistance(out->raw);
    out->curvature  = resistance_to_curvature(out->resistance);
    out->is_bent    = (out->curvature >= s_threshold);

    return ESP_OK;
}

esp_err_t flex_sensor_read_all(flex_reading_t out[FLEX_SENSOR_COUNT])
{
    for (uint8_t i = 0; i < FLEX_SENSOR_COUNT; i++) {
        esp_err_t err = flex_sensor_read(i, &out[i]);
        if (err != ESP_OK) return err;
    }
    return ESP_OK;
}

bool flex_sensor_both_bent(void)
{
    flex_reading_t readings[FLEX_SENSOR_COUNT];
    if (flex_sensor_read_all(readings) != ESP_OK) return false;

    return readings[0].is_bent && readings[1].is_bent;
}

void flex_sensor_set_r_div(float r_ohm)
{
    s_r_div = r_ohm;
    ESP_LOGI(TAG, "R_div atualizado para %.0f ohms", r_ohm);
}

void flex_sensor_set_r_range(float r_flat_ohm, float r_bent_ohm)
{
    s_r_flat = r_flat_ohm;
    s_r_bent = r_bent_ohm;
    ESP_LOGI(TAG, "R_range atualizado: flat=%.0f bent=%.0f", r_flat_ohm, r_bent_ohm);
}

void flex_sensor_set_threshold(float threshold)
{
    if (threshold < 0.0f) threshold = 0.0f;
    if (threshold > 1.0f) threshold = 1.0f;
    s_threshold = threshold;
    ESP_LOGI(TAG, "Threshold de curvatura atualizado para %.2f", threshold);
}

#pragma once

/*
  ╔══════════════════════════════════════════════════════════╗
  ║                         RAND                             ║
  ║                                                          ║
  ║  Leitura e conversão dos sensores de flexão.             ║
  ║                                                          ║
  ║  O sensor de flex é um resistor variável: quanto mais    ║
  ║  dobrado, maior a resistência. Ele forma um divisor de   ║
  ║  tensão com um resistor fixo (R_DIV), e o ESP32 lê a     ║
  ║  tensão resultante via ADC de 12 bits (0–4095).          ║
  ║                                                          ║
  ║                                                          ║
  ║  Sensores usados: GPIO 0 (flex 0) e GPIO 1 (flex 1)      ║
  ╚══════════════════════════════════════════════════════════╝
 */

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// ── Número de sensores de flex ──────────────────────────── 
#define FLEX_SENSOR_COUNT   2

// ── GPIOs dos sensores ──────────────────────────────────── 
#define FLEX_GPIO_0         0    // Flex sensor 0 → ADC1 canal 0 
#define FLEX_GPIO_1         1    // Flex sensor 1 → ADC1 canal 1 

// ── Parâmetros do circuito 
#define FLEX_VCC_MV         3300 // Tensão de alimentação em mV   
#define FLEX_ADC_MAX        4095 // Resolução 12 bits             
#define FLEX_ADC_VREF_MV    3300 // Tensão de referência do ADC   

/*
  Resistência do sensor de flex em repouso e dobrado (ohms).
  Consulte o datasheet do seu sensor para ajustar esses valores.
  Exemplo comum: 10kΩ plano, 30–110kΩ dobrado.
 */
#define FLEX_R_FLAT_OHM     10000   // Resistência em repouso (plano)  
#define FLEX_R_BENT_OHM     50000   // Resistência totalmente dobrado   

/*
  Resistor fixo do divisor de tensão (ohms).
  Escolha próximo da média entre FLAT e BENT para melhor linearidade.
  Ex: (10k + 50k) / 2 = 30k → use 33kΩ como valor comercial.
 */
#define FLEX_R_DIV_OHM      33000   // Resistor fixo do divisor      

/*
  Limiar de curvatura para considerar o sensor "dobrado" (0.0–1.0).
  0.0 = completamente plano, 1.0 = curvatura máxima.
  Ajuste conforme seu uso (0.5 = metade da curvatura máxima).
 */
#define FLEX_BEND_THRESHOLD 0.5f


/* ════════════════════════════════════════════════════════════
   TIPOS
   ════════════════════════════════════════════════════════════ */

/*
  Dados de um sensor de flex
 
   raw        → valor bruto do ADC (0–4095)
   resistance → resistência calculada em ohms
   curvature  → curvatura normalizada (0.0 plano … 1.0 máximo)
   is_bent    → true se curvatura ≥ FLEX_BEND_THRESHOLD
 */
typedef struct {
    uint16_t raw;
    float    resistance;
    float    curvature;
    bool     is_bent;
} flex_reading_t;


/* ════════════════════════════════════════════════════════════
   API
   ════════════════════════════════════════════════════════════ */

/*
  flex_sensor_init()
 
 Inicializa o ADC para os dois canais dos sensores de flex.
  Deve ser chamado uma vez no início do programa.
 */
esp_err_t flex_sensor_init(void);

/*
  flex_sensor_read()
 
  Lê um sensor pelo índice (0 ou 1) e retorna os dados calculados.
 
  Parâmetros:
    index   → 0 ou 1 (correspondente a GPIO 0 ou GPIO 1)
    out     → ponteiro para receber a leitura
 */
esp_err_t flex_sensor_read(uint8_t index, flex_reading_t *out);

/*
  flex_sensor_read_all()
 
  Lê os dois sensores de uma vez.
 
  Parâmetros:
    out     → array de FLEX_SENSOR_COUNT leituras
 */
esp_err_t flex_sensor_read_all(flex_reading_t out[FLEX_SENSOR_COUNT]);

/*
  flex_sensor_both_bent()
 
  Retorna true se AMBOS os sensores estiverem dobrados
  acima do limiar FLEX_BEND_THRESHOLD.
 
  Uso direto no BLE HID para habilitar os cliques L/R.
 */
bool flex_sensor_both_bent(void);

/*
  flex_sensor_set_r_div()
 
  Atualiza em tempo de execução o valor do resistor do divisor.
  Útil se você quiser calibrar sem recompilar.
 
  Parâmetros:
    r_ohm → novo valor em ohms
 */
void flex_sensor_set_r_div(float r_ohm);

/*
  flex_sensor_set_r_range()
 
  Atualiza os limites de resistência do sensor (plano e dobrado).
  Usado para normalizar a curvatura corretamente.
 
  Parâmetros:
    r_flat_ohm → resistência do sensor plano
    r_bent_ohm → resistência do sensor totalmente dobrado
 */
void flex_sensor_set_r_range(float r_flat_ohm, float r_bent_ohm);

/*
  flex_sensor_set_threshold()
 
  Atualiza o limiar de curvatura para is_bent em tempo de execução.
 
  Parâmetros:
    threshold → valor entre 0.0 e 1.0
 */
void flex_sensor_set_threshold(float threshold);

#ifdef __cplusplus
}
#endif

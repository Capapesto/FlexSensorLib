# FlexSensorLib



# Biblioteca de Sensores de Flexao (Flex Sensors) para ESP32

[cite_start]Esta biblioteca foi desenvolvida para realizar a leitura e o calculo de curvatura de ate dois sensores de flexao(EXPANSIVO) utilizando o conversor analogico-digital (ADC) do ESP32 atraves do framework ESP-IDF.

[cite_start]A biblioteca converte o valor bruto do ADC (0 a 4095) e calcula a resistencia real em Ohms [cite: 1][cite_start], alem de retornar a curvatura normalizada em uma escala de 0.0 (totalmente plano) a 1.0 (totalmente dobrado).

---------------------------------------

## Configuracao do Hardware

Para que o ESP32 possa ler o sensor de flexao, e necessario montar um circuito de divisor de tensao para cada sensor. 

### Esquema de ligacao por sensor:
1. Conecte um dos terminais do Sensor de Flexao ao pino de 3.3V do ESP32.
2. Conecte o outro terminal do Sensor de Flexao a um Resistor Fixo de 33kΩ.
3. Conecte a outra extremidade do Resistor Fixo ao GND (Negativo) do ESP32.
4. O sinal de leitura do ADC deve ser derivado do ponto de conexao entre o sensor de flexao e o resistor fixo:
   * [cite_start]**Sensor 0:** Conectar ao pino **GPIO 0** (ADC1 Canal 0).
   * [cite_start]**Sensor 1:** Conectar ao pino **GPIO 1** (ADC1 Canal 1).

---------------------------------------

## Como Integrar ao Codigo

### 1. Adicionar ao projeto
[cite_start]Copie a pasta deste componente para o diretorio `components` do seu projeto ESP-IDF. [cite_start]O arquivo `CMakeLists.txt` interno ja esta configurado para registrar as dependencias necessarias do `esp_adc`.

### 2. Exemplo de Implementacao

Abaixo esta um exemplo basico de utilizacao para inclusao no arquivo `main.c`:

```c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "flex_sensor.h"

void app_main(void)
{
    // Inicializa a unidade ADC e configura os canais dos sensores
    if (flex_sensor_init() != ESP_OK) {
        printf("Falha na inicializacao dos sensores.\n");
        return;
    }

    flex_reading_t leitura_sensor_0;

    while (1) {
        // Realiza a leitura do Sensor 0 (GPIO 0)
        if (flex_sensor_read(0, &leitura_sensor_0) == ESP_OK) {
            
            printf("--- SENSOR 0 ---\n");
            printf("Valor Bruto (ADC): %d\n", leitura_sensor_0.raw);
            printf("Resistencia: %.1f Ohms\n", leitura_sensor_0.resistance);
            printf("Curvatura: %.2f (Escala de 0.0 a 1.0)\n", leitura_sensor_0.curvature);
            
            if (leitura_sensor_0.is_bent) {
                printf("Status: Limiar de dobra atingido.\n");
            } else {
                printf("Status: Sensor em posicao plana.\n");
            }
        }

        // Verifica se ambos os sensores estao dobrados simultaneamente
        if (flex_sensor_both_bent()) {
            printf("Aviso: Ambos os sensores estao dobrados.\n");
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

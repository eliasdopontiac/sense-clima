#ifndef __SENSECLIMA_H__
#define __SENSECLIMA_H__

#include "MQTTClient.h" // Para o tipo MQTTClient
#include <stdbool.h>
#include "HT_Fsm.h"

// Declare a variável como extern para que outros arquivos possam acessá-la
extern MQTTClient mqttClient;

// Valor padrão do intervalo de sono em milissegundos (30 segundos)
#define DEFAULT_SLEEP_INTERVAL_MS 30000

// Tópico para receber o intervalo de sono
#define INTERVAL_TOPIC "hana/externo/senseclima/sensor03/interval"

// Variável global para o intervalo de sono
extern uint32_t current_sleep_interval_ms;

/**
 * @brief Inicializa o módulo SenseClima.
 * 
 * Carrega as configurações salvas na NVRAM, como o intervalo de sono.
 */
void SenseClima_Init(void);

/**
 * @brief Lê o sensor DHT22 e publica a temperatura e a umidade via MQTT.
 * 
 * Esta função tenta ler o sensor DHT22 várias vezes em caso de falha.
 * Após obter os dados ou após um número máximo de tentativas, publica
 * os valores ou uma mensagem de erro via MQTT.
 * 
 * @return void                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
 */
void SenseClima_PublishDHT22State(void);

/**
 * @brief Obtém o intervalo de sono atual em milissegundos.
 * 
 * @return uint32_t Intervalo de sono em milissegundos.
 */
uint32_t SenseClima_GetSleepInterval(void);

/**
 * @brief Define o intervalo de sono diretamente com um valor em milissegundos.
 * 
 * @param interval_ms Novo intervalo de sono em milissegundos.
 * @return bool Verdadeiro se o intervalo foi atualizado com sucesso.
 */
bool SenseClima_SetSleepIntervalValue(uint32_t interval_ms);

/**
 * @brief Define o intervalo de sono com base em uma mensagem recebida.
 * 
 * @param payload Mensagem recebida contendo o novo intervalo.
 * @param payload_len Tamanho da mensagem.
 * @return bool Verdadeiro se o intervalo foi atualizado com sucesso.
 */
bool SenseClima_SetSleepInterval(uint8_t *payload, uint8_t payload_len);

/**
 * @brief Callback para processar mensagens recebidas nos tópicos subscritos.
 * 
 * @param payload Conteúdo da mensagem.
 * @param payload_len Tamanho da mensagem.
 * @param topic Tópico da mensagem.
 * @param topic_len Tamanho do tópico.
 */
void SenseClima_MessageHandler(uint8_t *payload, uint8_t payload_len, 
                              uint8_t *topic, uint8_t topic_len);

#endif // __SENSECLIMA_H__
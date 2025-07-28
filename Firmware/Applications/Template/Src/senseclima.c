#include "senseclima.h"
#include "HT_DHT22.h"
#include "HT_MQTT_Api.h"
#include "HT_Sleep.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Declaracao externa da variavel global mqttClient definida em HT_Fsm.c
extern MQTTClient mqttClient;

// Numero maximo de tentativas para ler o sensor
#define MAX_DHT_READ_ATTEMPTS 5
// Intervalo entre tentativas em ms
#define DHT_READ_RETRY_INTERVAL 1000

// Chave para armazenar o intervalo de sono na NVRAM
#define SLEEP_INTERVAL_KEY 0x12345678

// Intervalo de sono atual em milissegundos (inicializado com o valor padrao)
uint32_t current_sleep_interval_ms = DEFAULT_SLEEP_INTERVAL_MS;
// Flag para indicar se o intervalo foi configurado via MQTT
static bool interval_configured_via_mqtt = false;

// Funcao para carregar o intervalo de sono da NVRAM
static void LoadSleepIntervalFromNVRAM(void) {
    // Nesta versao simplificada, usamos apenas a variavel global
    // A persistencia real na NVRAM seria implementada com a API especifica do hardware
    printf("Carregando intervalo de sono: %lu ms (valor padrao)\n", current_sleep_interval_ms);
}

// Funcao para salvar o intervalo de sono na NVRAM
static void SaveSleepIntervalToNVRAM(void) {
    // Nesta versao simplificada, apenas logamos o valor
    // A persistencia real na NVRAM seria implementada com a API especifica do hardware
    printf("Salvando intervalo de sono: %lu ms\n", current_sleep_interval_ms);
}

// Inicializa o módulo SenseClima
void SenseClima_Init(void) {
    // Carrega o intervalo de sono da NVRAM
    LoadSleepIntervalFromNVRAM();
}

uint32_t SenseClima_GetSleepInterval(void) {
    return current_sleep_interval_ms;
}

// Versao simplificada que aceita diretamente um valor numerico
bool SenseClima_SetSleepIntervalValue(uint32_t interval_ms) {
    printf("\n=== ATUALIZANDO INTERVALO DE SONO ===\n");
    
    // Verifica se o valor está dentro de limites razoaveis (1 segundo a 24 horas)
    // Reduzir o limite minimo para 50ms para permitir testes
    if (interval_ms < 50 || interval_ms > 86400000) {
        printf("Intervalo fora dos limites permitidos (50ms-86400000ms / 50ms a 24 horas)\n");
        printf("=== FIM DA ATUALIZACAO ===\n\n");
        return false;
    }
    
    // Salva o valor anterior para comparação
    uint32_t previous_interval = current_sleep_interval_ms;
    
    // Atualiza o intervalo
    current_sleep_interval_ms = interval_ms;
    interval_configured_via_mqtt = true;
    
    printf("Intervalo ATUALIZADO: %lu ms (%lu segundos)\n", 
           current_sleep_interval_ms, current_sleep_interval_ms / 1000);
    printf("Valor anterior: %lu ms (%lu segundos)\n", previous_interval, previous_interval / 1000);
    
    // Salva o novo intervalo na NVRAM
    SaveSleepIntervalToNVRAM();
    
    printf("=== FIM DA ATUALIZACAO ===\n\n");
    
    return true;
}

bool SenseClima_SetSleepInterval(uint8_t *payload, uint8_t payload_len) {
    // Cria uma cópia da mensagem para garantir que seja terminada em null
    char buffer[32] = {0};
    
    printf("\n=== PROCESSANDO NOVO INTERVALO ===\n");
    printf("Payload recebido: ");
    
    // Imprimir cada byte do payload em formato hexadecimal
    for (int i = 0; i < payload_len; i++) {
        printf("%02X ", payload[i]);
    }
    printf(" (tamanho: %d)\n", payload_len);
    
    // Verifica se o payload está vazio
    if (payload_len == 0) {
        printf("Payload vazio, nenhuma atualizacao necessaria\n");
        printf("=== FIM DO PROCESSAMENTO ===\n\n");
        return false;
    }
    
    // Verifica se o payload é muito grande
    if (payload_len >= sizeof(buffer)) {
        printf("Payload muito grande para o buffer\n");
        printf("=== FIM DO PROCESSAMENTO ===\n\n");
        return false;
    }
    
    // Copia o payload para o buffer e garante que termine com null
    memcpy(buffer, payload, payload_len);
    buffer[payload_len] = '\0';
    
    printf("Buffer: '%s'\n", buffer);
    
    // Limpa caracteres de controle e espaços no início e no final
    int start = 0;
    int end = strlen(buffer) - 1;
    
    // Remove espaços e caracteres de controle do início
    while (start <= end && (buffer[start] <= ' ' || buffer[start] == '\n' || buffer[start] == '\r')) {
        start++;
    }
    
    // Remove espaços e caracteres de controle do final
    while (end >= start && (buffer[end] <= ' ' || buffer[end] == '\n' || buffer[end] == '\r')) {
        end--;
    }
    
    // Se o buffer estiver vazio após a limpeza
    if (start > end) {
        printf("Buffer vazio apos limpeza\n");
        printf("=== FIM DO PROCESSAMENTO ===\n\n");
        return false;
    }
    
    // Cria uma nova string limpa
    char clean_buffer[32] = {0};
    int clean_len = 0;
    
    // Copia apenas os caracteres válidos para o novo buffer
    for (int i = start; i <= end && clean_len < sizeof(clean_buffer) - 1; i++) {
        // Verifica se é um dígito ou um ponto decimal
        if ((buffer[i] >= '0' && buffer[i] <= '9') || buffer[i] == '.') {
            clean_buffer[clean_len++] = buffer[i];
        }
    }
    clean_buffer[clean_len] = '\0';
    
    printf("Valor numerico: '%s'\n", clean_buffer);
    
    // Se o buffer limpo estiver vazio
    if (clean_len == 0) {
        printf("Nenhum numero encontrado\n");
        printf("=== FIM DO PROCESSAMENTO ===\n\n");
        return false;
    }
    
    // Converte a string para um número inteiro
    char *endptr;
    long interval_seconds = strtol(clean_buffer, &endptr, 10);
    
    // Verifica se a conversão foi bem-sucedida
    if (endptr == clean_buffer || (*endptr != '\0' && *endptr != '.')) {
        printf("Erro ao converter intervalo: formato invalido\n");
        printf("=== FIM DO PROCESSAMENTO ===\n\n");
        return false;
    }
    
    printf("Valor convertido: %ld segundos\n", interval_seconds);
    
    // Converter segundos para milissegundos
    bool success = SenseClima_SetSleepIntervalValue((uint32_t)(interval_seconds * 1000));
    return success;
}

void SenseClima_MessageHandler(uint8_t *payload, uint8_t payload_len, 
                              uint8_t *topic, uint8_t topic_len) {
    // Cria uma cópia do tópico para garantir que seja terminado em null
    char topic_buffer[64] = {0};
    char expected_topic[64] = {0};
    
    // Verifica se o tópico é muito grande
    if (topic_len >= sizeof(topic_buffer)) {
        printf("Topico muito grande para o buffer\n");
        return;
    }
    
    // Copia o tópico para o buffer e garante que termine com null
    memcpy(topic_buffer, topic, topic_len);
    topic_buffer[topic_len] = '\0';
    
    // Copia o tópico esperado para comparação
    strcpy(expected_topic, INTERVAL_TOPIC);
    
    printf("Topico recebido: '%s'\n", topic_buffer);
    printf("Mensagem: '%.*s'\n", payload_len, payload);
    
    // Verifica se é o tópico de intervalo - usando strcmp para comparação exata
    if (strcmp(topic_buffer, expected_topic) == 0) {
        printf("Topico de intervalo identificado!\n");
        if (SenseClima_SetSleepInterval(payload, payload_len)) {
            printf("Intervalo de sono atualizado com sucesso\n");
        } else {
            printf("Falha ao atualizar intervalo de sono\n");
        }
    }
}

void SenseClima_PublishDHT22State(void) {
    float temperature;
    float humidity;
    char temp_payload[16];
    char hum_payload[16];
    char error_payload[] = "error";
    int attempt = 0;
    bool publish_success = false;
    int mqtt_reconnect_attempts = 0;
    const int MAX_MQTT_RECONNECT_ATTEMPTS = 3;
    
    printf("\n=== LEITURA SENSOR DHT22 ===\n");
    
    // Tenta ler o sensor várias vezes
    for (attempt = 0; attempt < MAX_DHT_READ_ATTEMPTS; attempt++) {
        int dht_status = DHT22_Read(&temperature, &humidity);
        
        if (dht_status == 0) {
            // Leitura bem-sucedida, formata os valores
            int temp_int_x10 = (int)(temperature * 10);
            int hum_int_x10 = (int)(humidity * 10);

            snprintf(temp_payload, sizeof(temp_payload), "%d.%d", temp_int_x10 / 10, temp_int_x10 % 10);
            snprintf(hum_payload, sizeof(hum_payload), "%d.%d", hum_int_x10 / 10, hum_int_x10 % 10);

            printf("Leitura OK: Temp=%sC, Umid=%s%%\n", temp_payload, hum_payload);
            break;
        } else {
            printf("Tentativa %d: Erro na leitura (codigo: %d)\n", attempt + 1, dht_status);
            
            if (attempt < MAX_DHT_READ_ATTEMPTS - 1) {
                // Aguarda antes da próxima tentativa
                osDelay(DHT_READ_RETRY_INTERVAL);
            }
        }
    }
    
    // Verifica se conseguiu ler o sensor
    if (attempt >= MAX_DHT_READ_ATTEMPTS) {
        printf("Falha na leitura apos %d tentativas\n", MAX_DHT_READ_ATTEMPTS);
        // Usa a mensagem de erro para ambas as publicações
        strcpy(temp_payload, error_payload);
        strcpy(hum_payload, error_payload);
    }
    
    // Tenta publicar os dados (com retry se necessário)
    for (int retry = 0; retry < 3; retry++) {
        // Verifica se o cliente MQTT está conectado
        if (!mqttClient.isconnected) {
            printf("MQTT desconectado. Tentando reconectar...\n");
            
            // Tenta reconectar ao MQTT até o número máximo de tentativas
            while (mqtt_reconnect_attempts < MAX_MQTT_RECONNECT_ATTEMPTS && !mqttClient.isconnected) {
                printf("Tentativa de reconexao %d de %d...\n", 
                       mqtt_reconnect_attempts + 1, MAX_MQTT_RECONNECT_ATTEMPTS);
                
                // Tenta reconectar ao MQTT
                if (HT_FSM_MQTTConnect() == HT_CONNECTED) {
                    printf("MQTT reconectado com sucesso!\n");
                    break;
                } else {
                    mqtt_reconnect_attempts++;
                    
                    if (mqtt_reconnect_attempts < MAX_MQTT_RECONNECT_ATTEMPTS) {
                        printf("Aguardando 3 segundos...\n");
                        osDelay(3000);
                    } else {
                        printf("Falha na reconexao apos %d tentativas\n", MAX_MQTT_RECONNECT_ATTEMPTS);
                    }
                }
            }
            
            // Se ainda não está conectado após todas as tentativas, pula esta iteração
            if (!mqttClient.isconnected) {
                continue;
            }
        }
        
        // Publica temperatura
        printf("Publicando temperatura: %s\n", temp_payload);
        HT_MQTT_Publish(&mqttClient, "hana/externo/senseclima/sensor03/temperature", 
                       (uint8_t *)temp_payload, strlen(temp_payload), QOS0, 0, 0, 0);
        
        // Publica umidade
        printf("Publicando umidade: %s\n", hum_payload);
        HT_MQTT_Publish(&mqttClient, "hana/externo/senseclima/sensor03/humidity", 
                       (uint8_t *)hum_payload, strlen(hum_payload), QOS0, 0, 0, 0);
        
        printf("Dados publicados com sucesso!\n");
        publish_success = true;
        break;
    }
    
    if (!publish_success) {
        printf("Nao foi possivel publicar os dados\n");
    }
    
    printf("=== FIM DA LEITURA E PUBLICACAO ===\n");
}
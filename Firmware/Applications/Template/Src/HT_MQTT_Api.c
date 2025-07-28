/**
 *
 * Copyright (c) 2023 HT Micron Semicondutores S.A.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "HT_MQTT_Api.h"
#include "HT_Fsm.h"
#include "HT_MQTT_Tls.h"
#include "senseclima.h"

extern volatile uint8_t subscribe_callback;

static MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;

#if  MQTT_TLS_ENABLE == 1
static MqttClientContext mqtt_client_ctx;
#endif

uint8_t HT_MQTT_Connect(MQTTClient *mqtt_client, Network *mqtt_network, char *addr, int32_t port, uint32_t send_timeout, uint32_t rcv_timeout, char *clientID, 
                                        char *username, char *password, uint8_t mqtt_version, uint32_t keep_alive_interval, uint8_t *sendbuf, 
                                        uint32_t sendbuf_size, uint8_t *readbuf, uint32_t readbuf_size) {


#if  MQTT_TLS_ENABLE == 1
    mqtt_client_ctx.caCertLen = 0;
    mqtt_client_ctx.port = port;
    mqtt_client_ctx.host = addr;
    mqtt_client_ctx.timeout_ms = MQTT_GENERAL_TIMEOUT;
    mqtt_client_ctx.isMqtt = true;
    mqtt_client_ctx.timeout_r = MQTT_GENERAL_TIMEOUT;
    mqtt_client_ctx.timeout_s = MQTT_GENERAL_TIMEOUT;
#endif

    connectData.MQTTVersion = mqtt_version;
    connectData.clientID.cstring = clientID;
    connectData.username.cstring = username;
    connectData.password.cstring = password;
    connectData.keepAliveInterval = keep_alive_interval;
    
    // Nao configurar a mensagem de Last Will and Testament (LWT)
    connectData.willFlag = 0;  // Desabilitar LWT
    
    // Usar cleansession=true para garantir uma sessao limpa a cada conexao
    connectData.cleansession = false;

#if  MQTT_TLS_ENABLE == 1

    printf("Starting TLS handshake...\n");

    if(HT_MQTT_TLSConnect(&mqtt_client_ctx, mqtt_network) != 0) {
        printf("TLS Connection Error!\n");
        return 1;
    }

    MQTTClientInit(mqtt_client, mqtt_network, MQTT_GENERAL_TIMEOUT, (unsigned char *)sendbuf, sendbuf_size, (unsigned char *)readbuf, readbuf_size);

    if ((MQTTConnect(mqtt_client, &connectData)) != 0) {
        mqtt_client->ping_outstanding = 1;
        return 1;
    } else {
        mqtt_client->ping_outstanding = 0;
    }

#else

    NetworkInit(mqtt_network);
    MQTTClientInit(mqtt_client, mqtt_network, MQTT_GENERAL_TIMEOUT, (unsigned char *)sendbuf, sendbuf_size, (unsigned char *)readbuf, readbuf_size);
    
    if((NetworkSetConnTimeout(mqtt_network, send_timeout, rcv_timeout)) != 0) {
        mqtt_client->keepAliveInterval = connectData.keepAliveInterval;
        mqtt_client->ping_outstanding = 1;

    } else {
        
        if ((NetworkConnect(mqtt_network, addr, port)) != 0) {
            mqtt_client->keepAliveInterval = connectData.keepAliveInterval;
            mqtt_client->ping_outstanding = 1;
            
            return 1;

        } else {
            if ((MQTTConnect(mqtt_client, &connectData)) != 0) {
                mqtt_client->ping_outstanding = 1;
                return 1;
    
            } else {
                mqtt_client->ping_outstanding = 0;
            }
        }

        if(mqtt_client->ping_outstanding == 0) {
            if ((MQTTStartRECVTask(mqtt_client)) != SUCCESS){
                return 1;
            }
        }
    }

#endif

    return 0;
}

void HT_MQTT_Publish(MQTTClient *mqtt_client, char *topic, uint8_t *payload, uint32_t len, enum QoS qos, uint8_t retained, uint16_t id, uint8_t dup) {
    MQTTMessage message;

    message.qos = qos;
    message.retained = retained;
    message.id = id;
    message.dup = dup;
    message.payload = payload;
    message.payloadlen = len;

    MQTTPublish(mqtt_client, topic, &message);
}

void HT_MQTT_SubscribeCallback(MessageData *msg) {
    // Criar copias terminadas em null das strings para evitar problemas
    char *payload_str = malloc(msg->message->payloadlen + 1);
    char *topic_str = malloc(msg->topicName->lenstring.len + 1);
    
    if (!payload_str || !topic_str) {
        printf("Erro de alocacao de memoria no callback MQTT\n");
        if (payload_str) free(payload_str);
        if (topic_str) free(topic_str);
        return;
    }
    
    // Copiar o payload e o topico para as strings alocadas
    memcpy(payload_str, msg->message->payload, msg->message->payloadlen);
    payload_str[msg->message->payloadlen] = '\0';
    
    memcpy(topic_str, msg->topicName->lenstring.data, msg->topicName->lenstring.len);
    topic_str[msg->topicName->lenstring.len] = '\0';
    
    printf("\n=== MENSAGEM MQTT RECEBIDA ===\n");
    printf("Topico: '%s'\n", topic_str);
    printf("Tamanho: %d bytes\n", msg->message->payloadlen);
    printf("Payload: '%s'\n", payload_str);
    
    // Verificar se é o topico de intervalo - usando strcmp para comparacao exata
    if (strcmp(topic_str, INTERVAL_TOPIC) == 0) {
        printf("TOPICO DE INTERVALO DETECTADO!\n");
        
        // Tentar processar o payload, independentemente do formato
        if (SenseClima_SetSleepInterval((uint8_t*)payload_str, (uint8_t)msg->message->payloadlen)) {
            printf("INTERVALO ATUALIZADO COM SUCESSO!\n");
        } else {
            printf("FALHA AO ATUALIZAR INTERVALO: valor invalido\n");
            
            // Tentar extrair um numero do payload, mesmo que esteja em formato XML ou JSON
            char *p = payload_str;
            char number_buffer[32] = {0};
            int number_len = 0;
            bool found_digit = false;
            
            // Procurar por digitos no payload
            while (*p && number_len < sizeof(number_buffer) - 1) {
                if (*p >= '0' && *p <= '9') {
                    found_digit = true;
                    number_buffer[number_len++] = *p;
                } else if (found_digit && *p == '.') {
                    // Permitir um ponto decimal se ja encontramos digitos
                    number_buffer[number_len++] = *p;
                } else if (found_digit && (*p < '0' || *p > '9')) {
                    // Terminar a extracao se encontramos um nao-digito apos digitos
                    break;
                }
                p++;
            }
            
            if (number_len > 0) {
                number_buffer[number_len] = '\0';
                printf("Numero extraido: '%s'\n", number_buffer);
                
                // Tentar converter o numero extraido
                char *endptr;
                long interval_value = strtol(number_buffer, &endptr, 10);
                
                if (endptr != number_buffer && (*endptr == '\0' || *endptr == '.') && interval_value > 0) {
                    // Converter segundos para milissegundos
                    uint32_t new_interval_ms = (uint32_t)(interval_value * 1000);
                    printf("SEGUNDA TENTATIVA - NOVO INTERVALO: %lu ms (%ld segundos)\n", 
                           new_interval_ms, interval_value);
                    
                    // Atualizar o intervalo de sono usando a funcao direta
                    if (SenseClima_SetSleepIntervalValue(new_interval_ms)) {
                        printf("SEGUNDA TENTATIVA - INTERVALO ATUALIZADO!\n");
                    } else {
                        printf("SEGUNDA TENTATIVA - FALHA NA ATUALIZACAO\n");
                    }
                }
            }
        }
    }
    
    // Processa a mensagem usando o handler do SenseClima
    SenseClima_MessageHandler((uint8_t *)payload_str, 
                             (uint8_t)strlen(payload_str),
                             (uint8_t *)topic_str, 
                             (uint8_t)strlen(topic_str));
    
    // Para compatibilidade com o codigo existente
    subscribe_callback = 1;
    HT_FSM_SetSubscribeBuff((uint8_t *)payload_str, (uint8_t)strlen(payload_str));
    
    // Limpar a memoria alocada
    free(payload_str);
    free(topic_str);
    
    printf("=== FIM DO PROCESSAMENTO MQTT ===\n\n");
}

void HT_MQTT_Subscribe(MQTTClient *mqtt_client, char *topic, enum QoS qos) {
    MQTTSubscribe(mqtt_client, (const char *)topic, qos, HT_MQTT_SubscribeCallback);
}

/************************ HT Micron Semicondutores S.A *****END OF FILE****/
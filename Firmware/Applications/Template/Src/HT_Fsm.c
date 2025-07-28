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

#include "HT_Fsm.h"
#include "HT_DHT22.h"
#include "senseclima.h"
#include "HT_Sleep.h"

/* Declaracoes externas ------------------------------------------------------------------*/
extern void HT_LED_GreenLedTask(void *arg);

/* Function prototypes  ------------------------------------------------------------------*/

/*!******************************************************************
 * \fn static void HT_YieldThread(void *arg)
 * \brief Thread created as MQTT background.
 *
 * \param[in]  void *arg                    Thread parameter.
 * \param[out] none
 *
 * \retval none.
 *******************************************************************/
static void HT_YieldThread(void *arg);

/*!******************************************************************
 * \fn static void HT_Yield_Thread(void *arg)
 * \brief Creates a thread that will be the MQTT background.
 *
 * \param[in]  void *arg                    Parameters that will be used in the created thread.
 * \param[out] none
 *
 * \retval none.
 *******************************************************************/
static void HT_Yield_Thread(void *arg);

/*!******************************************************************
 * \fn static void HT_FSM_MQTTWritePayload(uint8_t *ptr, uint8_t size)
 * \brief Copy the *ptr content to the mqtt_payload.
 *
 * \param[in]  uint8_t *ptr                 Pointer with the content that will be copied.
 * \param[in]  uint8_t size                 Buffer size.
 * \param[out] none
 *
 * \retval none.
 *******************************************************************/
static void HT_FSM_MQTTWritePayload(uint8_t *ptr, uint8_t size);

/*!******************************************************************
 * \fn static void HT_FSM_UpdateUserLedState(void)
 * \brief Get the last led state from the digital twin. If the digital
 * twin has one of these leds on, it retrieves its state by turning on
 * the respective led on-board.
 *
 * \param[in]  none
 * \param[out] none
 *
 * \retval none.
 *******************************************************************/
void HT_FSM_UpdateUserLedState(void);

/*!******************************************************************
 * \fn static void HT_FSM_LedStatus(HT_Led_Type led, uint16_t state)
 * \brief Change a specific led status to ON/OFF.
 *
 * \param[in]  HT_Led_Type led              LED id.
 * \param[in]  uint16_t state               LED state (ON/OFF)
 * \param[out] none
 *
 * \retval none.
 *******************************************************************/
static void HT_FSM_LedStatus(HT_Led_Type led, uint16_t state);

/*!******************************************************************
 * \fn HT_ConnectionStatus HT_FSM_MQTTConnect(void)
 * \brief Connects the device to the MQTT Broker and returns the connection
 * status.
 *
 * \param[in]  none
 * \param[out] none
 *
 * \retval Connection status.
 *******************************************************************/
HT_ConnectionStatus HT_FSM_MQTTConnect(void);

/*!******************************************************************
 * \fn static void HT_FSM_SubscribeHandleState(void)
 * \brief Subscribe Handle State implementation. Process a subscribe
 * received event that comes from the digital twin.
 *
 * \param[in]  none
 * \param[out] none
 *
 * \retval none.
 *******************************************************************/
static void HT_FSM_SubscribeHandleState(void);

/*!******************************************************************
 * \fn static void HT_FSM_MQTTPublishState(void)
 * \brief MQTT Publish State implementation. Publishes a MQTT payload to
 * a respective topic. This payload are data containing information
 * about push button events and commands that are supposed to be process
 * by the digital twin.
 *
 * \param[in]  none
 * \param[out] none
 *
 * \retval none.
 *******************************************************************/
static void HT_FSM_MQTTPublishState(void);

/*!******************************************************************
 * \fn static void HT_FSM_MQTTSubscribeState(void)
 * \brief MQTT Subscribe State implementation. Subscribe to the MQTT topics
 * that are supposed to transmit the commands coming from the digital twin.
 *
 * \param[in]  none
 * \param[out] none
 *
 * \retval none.
 *******************************************************************/
static void HT_FSM_MQTTSubscribeState(void);

/*!******************************************************************
 * \fn static void HT_FSM_PushButtonHandleState(void)
 * \brief Push Button Handle State implementation. Process a push button
 * event by turning on the respective led and also publishing the respective 
 * button color to the MQTT broker.
 *
 * \param[in]  none
 * \param[out] none
 *
 * \retval none.
 *******************************************************************/
static void HT_FSM_PushButtonHandleState(void);

/*!******************************************************************
 * \fn static void HT_FSM_WaitForButtonState(void)
 * \brief Wait For Button State implementation. Keeps waiting until
 * an user button (blue or white button) is pressed and sets the FSM
 * to Push Button Handle states after a push button event.
 *
 * \param[in]  none
 * \param[out] none
 *
 * \retval none.
 *******************************************************************/
static void HT_FSM_WaitForButtonState(void);

/*!******************************************************************
 * \fn static void HT_FSM_CheckSocketState(void)
 * \brief Check Socket State implementation. Waits until the 
 * subscribe callback is called and sets the FSM to Wait For Button State
 * after that.
 *
 * \param[in]  none
 * \param[out] none
 *
 * \retval none.
 *******************************************************************/
static void HT_FSM_CheckSocketState(void);

/*!******************************************************************
 * \fn static void HT_FSM_MQTTPublishDHT22State(void)
 * \brief Reads the DHT22 sensor, publishes the data via MQTT and
 * sets the FSM to enter deep sleep.
 *
 * \param[in]  none
 * \param[out] none
 *
 * \retval none.
 *******************************************************************/
static void HT_FSM_MQTTPublishDHT22State(void);

/*!******************************************************************
 * \fn static void HT_FSM_EnterDeepSleepState(void)
 * \brief Enters deep sleep mode for a predefined interval.
 *
 * \param[in]  none
 * \param[out] none
 *
 * \retval none.
 *******************************************************************/
static void HT_FSM_EnterDeepSleepState(void);

/* ---------------------------------------------------------------------------------------*/

MQTTClient mqttClient;
Network mqttNetwork;

//Buffer that will be published.
static uint8_t mqtt_payload[128] = {"Undefined Button"};
static uint8_t mqttSendbuf[HT_MQTT_BUFFER_SIZE] = {0};
static uint8_t mqttReadbuf[HT_MQTT_BUFFER_SIZE] = {0};

static const char clientID[] = {"SIP_HTNB32L"};
static const char username[] = {""};
static const char password[] = {""};

//MQTT broker host address
static const char addr[] = {"131.255.82.115"}; //{"0.tcp.sa.ngrok.io"};//;
static char topic[25] = {0};

// Blue button topic where the digital twin will transmit its messages.
char topic_bluebutton_sw[] = {"hana/danilo/htnb32l_bluebutton_sw"};

// White button topic where the digital twin will transmit its messages.
char topic_whitebutton_sw[] = {"hana/danilo/htnb32l_whitebutton_sw"};

static const char blue_button_str[] = {"Blue"};
static const char white_button_str[] = {"White"};

//FSM state.
volatile HT_FSM_States state = HT_WAIT_FOR_BUTTON_STATE;

//Button color definition.
volatile HT_Button button_color = HT_UNDEFINED;

//Subcribe callback flag
volatile uint8_t subscribe_callback = 0;

static HT_Button prev_color;

//Button IRQn flag
extern volatile uint8_t button_irqn; 

extern uint16_t blue_irqn_mask;
extern uint16_t white_irqn_mask;

//Buffer where the digital twin messages will be stored.
static uint8_t subscribe_buffer[HT_SUBSCRIBE_BUFF_SIZE] = {0};

static volatile uint8_t blue_button_state = 0;
static volatile uint8_t white_button_state = 0;

static StaticTask_t yield_thread;
static uint8_t yieldTaskStack[1024*4];

static void HT_YieldThread(void *arg) {
    while (1) {
        // Wait function for 10ms to check if some message arrived in subscribed topic
        MQTTYield(&mqttClient, 10);
    }
}

static void HT_Yield_Thread(void *arg) {
    osThreadAttr_t task_attr;

    memset(&task_attr,0,sizeof(task_attr));
    memset(yieldTaskStack, 0xA5,LED_TASK_STACK_SIZE);
    task_attr.name = "yield_thread";
    task_attr.stack_mem = yieldTaskStack;
    task_attr.stack_size = LED_TASK_STACK_SIZE;
    task_attr.priority = osPriorityNormal;
    task_attr.cb_mem = &yield_thread;
    task_attr.cb_size = sizeof(StaticTask_t);

    osThreadNew(HT_YieldThread, NULL, &task_attr);
}

static void HT_FSM_MQTTWritePayload(uint8_t *ptr, uint8_t size) {
    // Reset payload and writes the message
    memset(mqtt_payload, 0, sizeof(mqtt_payload));
    memcpy(mqtt_payload, ptr, size);
}

void HT_FSM_UpdateUserLedState(void) {
    char payload[] = {"GetState"};

    HT_FSM_MQTTWritePayload((uint8_t *)payload, strlen(payload));
    memset(topic, 0, sizeof(topic));
    sprintf(topic, "hana/danilo/htnb32l_get_state");

    HT_MQTT_Publish(&mqttClient, (char *)topic, mqtt_payload, strlen((char *)mqtt_payload), QOS0, 0, 0, 0);

    memset(topic, 0, sizeof(topic));
    sprintf(topic, "hana/danilo/htnb32l_set_state");
    
    HT_MQTT_Subscribe(&mqttClient, topic, QOS0);
    HT_MQTT_Subscribe(&mqttClient, topic_bluebutton_sw, QOS0);
    HT_MQTT_Subscribe(&mqttClient, topic_whitebutton_sw, QOS0);

    HT_Yield_Thread(NULL);
    printf("Inscricoes iniciais enviadas. FSM processara as respostas.\n");

    // REMOVED: Blocking 'while(!subscribe_callback)' loop.
    // The FSM will now proceed to its main loop and handle incoming
    // messages and sensor readings asynchronously.
    // We assume the initial LED state is OFF.
    HT_GPIO_WritePin(BLUE_LED_PIN, BLUE_LED_INSTANCE, LED_OFF);
    blue_button_state = 0;
    HT_GPIO_WritePin(WHITE_LED_PIN, WHITE_LED_INSTANCE, LED_OFF);
    white_button_state = 0;
}

static void HT_FSM_LedStatus(HT_Led_Type led, uint16_t state) {

    // Turns on/off selected led
    switch (led) {
    case HT_BLUE_LED:
        HT_GPIO_WritePin(BLUE_LED_PIN, BLUE_LED_INSTANCE, state);
        break;
    case HT_WHITE_LED:
        HT_GPIO_WritePin(WHITE_LED_PIN, WHITE_LED_INSTANCE, state);
        break;
    case HT_GREEN_LED:
        HT_GPIO_WritePin(GREEN_LED_PIN, GREEN_LED_INSTANCE, state);
        break;
    }
}

HT_ConnectionStatus HT_FSM_MQTTConnect(void) {

    // Connect to MQTT Broker using client, network and parameters needded. 
    if(HT_MQTT_Connect(&mqttClient, &mqttNetwork, (char *)addr, HT_MQTT_PORT, HT_MQTT_SEND_TIMEOUT, HT_MQTT_RECEIVE_TIMEOUT,
                (char *)clientID, (char *)username, (char *)password, HT_MQTT_VERSION, HT_MQTT_KEEP_ALIVE_INTERVAL, mqttSendbuf, HT_MQTT_BUFFER_SIZE, mqttReadbuf, HT_MQTT_BUFFER_SIZE)) {
        return HT_NOT_CONNECTED;   
    }

    printf("MQTT Connection Success!\n");

    return HT_CONNECTED;
}

void HT_FSM_SetSubscribeBuff(uint8_t *buff, uint8_t payload_len) {
    memcpy(subscribe_buffer, buff, payload_len);
}

static void HT_FSM_SubscribeHandleState(void) {
    
    if(!strncmp((char *)subscribe_buffer, blue_button_str, strlen(blue_button_str))){
        blue_button_state ^= 1;
        HT_FSM_LedStatus(HT_BLUE_LED, blue_button_state ? LED_ON : LED_OFF);
    } else if(!strncmp((char *)subscribe_buffer, white_button_str, strlen(white_button_str))) {
        white_button_state ^= 1;
        HT_FSM_LedStatus(HT_WHITE_LED, white_button_state ? LED_ON : LED_OFF);
    }

    state = HT_WAIT_FOR_BUTTON_STATE;
}

static void HT_FSM_MQTTPublishDHT22State(void) {
    // Chama a função que lê e publica os dados do sensor
    SenseClima_PublishDHT22State();

    // Define o próximo estado como entrar em sono profundo
    state = HT_ENTER_DEEP_SLEEP_STATE;
}

static void HT_FSM_EnterDeepSleepState(void) {
    // Obtem o intervalo de sono atual (configurado via MQTT ou valor padrao)
    const uint32_t sleep_duration_ms = SenseClima_GetSleepInterval();
    printf("\n=== PREPARANDO PARA HIBERNACAO ===\n");
    printf("Intervalo configurado: %lu ms (%lu segundos)\n", 
           sleep_duration_ms, sleep_duration_ms / 1000);
    
    // Desconecta do MQTT para limpar recursos
    if (mqttClient.isconnected) {
        printf("Desconectando do MQTT antes de dormir...\n");
        MQTTDisconnect(&mqttClient);
    }
    
    // Desativa todos os perifericos que possam impedir o sono profundo
    printf("Desativando perifericos...\n");
    
    // Desativa LEDs
    HT_FSM_LedStatus(HT_BLUE_LED, LED_OFF);
    HT_FSM_LedStatus(HT_WHITE_LED, LED_OFF);
    HT_FSM_LedStatus(HT_GREEN_LED, LED_OFF);

    // Aguarda um momento para garantir que todas as operacoes sejam concluidas
    osDelay(500);
    
    // Entra no modo de hibernacao - esta funcao nao retorna
    printf("Entrando em hibernacao...\n");
    HT_Sleep_EnterSleep(SLP_HIB_STATE, sleep_duration_ms);
    
    // Este codigo nunca sera alcancado
}

static void HT_FSM_MQTTPublishState(void) {

    // Publishes payload defined from the button color with QOS 0 and not retain message
    printf("Publicando...\n");
    HT_MQTT_Publish(&mqttClient, (char *)topic, mqtt_payload, strlen((char *)mqtt_payload), QOS0, 0, 0, 0);

    osDelay(500);
    GPIO_RestoreIRQMask(BLUE_BUTTON_INSTANCE, blue_irqn_mask);
    GPIO_RestoreIRQMask(WHITE_BUTTON_INSTANCE, white_irqn_mask);

    prev_color = HT_UNDEFINED;
    state = HT_WAIT_FOR_BUTTON_STATE;
}

static void HT_FSM_MQTTSubscribeState(void) {
    
    // Subscribe to defined topic with QOS 0
    printf("Inscrevendo nos topicos...\n");
    HT_MQTT_Subscribe(&mqttClient, topic_bluebutton_sw, QOS0);
    HT_MQTT_Subscribe(&mqttClient, topic_whitebutton_sw, QOS0);

    printf("Inscricao concluida!\n");
    
    // Change state to wait for button interruption
    state = HT_WAIT_FOR_BUTTON_STATE;
}

static void HT_FSM_PushButtonHandleState(void) {

    // Turns on led and write payload according to the color of the pressed button
    switch (button_color) {
    case HT_BLUE_BUTTON:
        printf("Botao AZUL pressionado!\n");
        
        blue_button_state ^= 1;
        HT_FSM_LedStatus(HT_BLUE_LED, blue_button_state ? LED_ON : LED_OFF);

        HT_FSM_MQTTWritePayload((uint8_t *)blue_button_str, strlen(blue_button_str));

        memset(topic, 0, sizeof(topic));
        sprintf(topic, "hana/danilo/htnb32l_bluebutton_fw");

        // Change to publish state
        state = HT_MQTT_PUBLISH_STATE;
        break;
    case HT_WHITE_BUTTON:
        printf("Botao BRANCO pressionado!\n");
        
        white_button_state ^= 1;
        HT_FSM_LedStatus(HT_WHITE_LED, white_button_state ? LED_ON : LED_OFF);

        HT_FSM_MQTTWritePayload((uint8_t *)white_button_str, strlen(white_button_str));

        memset(topic, 0, sizeof(topic));
        sprintf(topic, "hana/danilo/htnb32l_whitebutton_fw");
        
        // Change to publish state
        state = HT_MQTT_PUBLISH_STATE;
        break;
    // Case something not expected happened, print error and change state to wait for button interruption
    case HT_UNDEFINED:
        printf("ERRO! Cor do botao indefinida!\n");
        state = HT_WAIT_FOR_BUTTON_STATE;
        break;
    }    

    // Reset button_irqn and button_color
    button_irqn = 0;
    prev_color = button_color;
}

static void HT_FSM_WaitForButtonState(void) {
    // If some button was pressed, change to the next state, else keep the same state
    state = !button_irqn ? HT_CHECK_SOCKET_STATE : HT_PUSH_BUTTON_HANDLE_STATE;
}

static void HT_FSM_CheckSocketState(void) {
    state = subscribe_callback ? HT_SUBSCRIBE_HANDLE_STATE : HT_WAIT_FOR_BUTTON_STATE;
    subscribe_callback = 0;
}

// Função removida: CheckForIntervalMessages

void HT_Fsm(void) {
    int mqtt_connect_attempts = 0;
    const int MAX_MQTT_CONNECT_ATTEMPTS = 3;
    bool mqtt_connected = false;

    // Inicializa o sensor DHT22
    DHT22_Init();
    
    // Inicializa o módulo SenseClima (carrega configurações da NVRAM)
    SenseClima_Init();
    
    printf("Intervalo de sono: %lu ms\n", SenseClima_GetSleepInterval());

    // Loop para tentar conectar ao MQTT até o número máximo de tentativas
    while (mqtt_connect_attempts < MAX_MQTT_CONNECT_ATTEMPTS && !mqtt_connected) {
        printf("\nTentativa de conexao MQTT %d de %d...\n", mqtt_connect_attempts + 1, MAX_MQTT_CONNECT_ATTEMPTS);
        
        // Tenta conectar ao MQTT
        if (HT_FSM_MQTTConnect() == HT_CONNECTED) {
            mqtt_connected = true;
            printf("MQTT conectado com sucesso!\n");
        } else {
            mqtt_connect_attempts++;
            printf("Falha na conexao MQTT (tentativa %d de %d)\n", mqtt_connect_attempts, MAX_MQTT_CONNECT_ATTEMPTS);
            
            if (mqtt_connect_attempts < MAX_MQTT_CONNECT_ATTEMPTS) {
                // Aguarda antes de tentar novamente
                printf("Aguardando 5 segundos...\n");
                osDelay(5000);
            }
        }
    }

    // Se não conseguiu conectar após todas as tentativas, entra em modo de hibernação
    if (!mqtt_connected) {
        printf("Nao foi possivel conectar ao MQTT apos %d tentativas.\n", MAX_MQTT_CONNECT_ATTEMPTS);
        printf("Entrando em hibernacao...\n");
        
        // Desativa LEDs antes de dormir
        HT_FSM_LedStatus(HT_BLUE_LED, LED_OFF);
        HT_FSM_LedStatus(HT_WHITE_LED, LED_OFF);
        HT_FSM_LedStatus(HT_GREEN_LED, LED_OFF);
        
        // Entra em hibernação usando o intervalo padrão ou configurado
        HT_Sleep_EnterSleep(SLP_HIB_STATE, SenseClima_GetSleepInterval());
        
        // Este código nunca será alcançado
        return;
    }

    // Se chegou aqui, a conexão MQTT foi bem-sucedida
    // Init irqn after connection
    HT_GPIO_ButtonInit();

    // Led to sinalize connection stablished
    HT_LED_GreenLedTask(NULL);

    printf("Executando FSM...\n");

    // Subscreve ao tópico de intervalo
    printf("Inscrevendo no topico: '%s' com QoS 1\n", INTERVAL_TOPIC);
    HT_MQTT_Subscribe(&mqttClient, INTERVAL_TOPIC, QOS1);
    printf("Inscricao enviada\n");

    // Usa o tick count do FreeRTOS para um controle de tempo mais preciso.
    TickType_t last_dht_read_time = xTaskGetTickCount();
    const TickType_t dht_read_interval = pdMS_TO_TICKS(30000); // 30 segundos

    // Inicia imediatamente com a leitura do sensor e publicação
    state = HT_MQTT_PUBLISH_DHT22_STATE;

    while (1) {
        switch (state) {
            case HT_CHECK_SOCKET_STATE:
                // Check if some message arrived
                HT_FSM_CheckSocketState();
                break;
            case HT_WAIT_FOR_BUTTON_STATE:
                // Check if some button was pressed
                HT_FSM_WaitForButtonState();
                // Verifica se já passou o tempo para ler o sensor DHT22.
                if ((xTaskGetTickCount() - last_dht_read_time) >= dht_read_interval) {
                    state = HT_MQTT_PUBLISH_DHT22_STATE;
                    last_dht_read_time = xTaskGetTickCount(); // Reinicia o contador de tempo.
                }
                break;
            case HT_PUSH_BUTTON_HANDLE_STATE:
                // Defines which button was pressed
                HT_FSM_PushButtonHandleState();
                break;
            case HT_MQTT_SUBSCRIBE_STATE:
                // Subscribe to MQTT Topic defined in global variables
                HT_FSM_MQTTSubscribeState();
                break;
            case HT_MQTT_PUBLISH_STATE:
                // Publish MQTT Message: pressed button color
                HT_FSM_MQTTPublishState();
                break;
            case HT_SUBSCRIBE_HANDLE_STATE:
                HT_FSM_SubscribeHandleState();
                break;
            case HT_MQTT_PUBLISH_DHT22_STATE:
                HT_FSM_MQTTPublishDHT22State();
                break;
            case HT_ENTER_DEEP_SLEEP_STATE:
                HT_FSM_EnterDeepSleepState();
                break;
            default:
                break;
            }
            osDelay(pdMS_TO_TICKS(100));
    }
}

/************************ HT Micron Semicondutores S.A *****END OF FILE****/

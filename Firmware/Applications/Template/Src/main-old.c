#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "slpman_qcx212.h"
#include "pad_qcx212.h"
#include "HT_gpio_qcx212.h"
#include "ic_qcx212.h"
#include "HT_ic_qcx212.h"

static uint32_t uart_cntrl = (ARM_USART_MODE_ASYNCHRONOUS | ARM_USART_DATA_BITS_8 | ARM_USART_PARITY_NONE | 
                                ARM_USART_STOP_BITS_1 | ARM_USART_FLOW_CONTROL_NONE);

extern USART_HandleTypeDef huart1;

//GPIO10 - BUTTON
#define BUTTON_INSTANCE          0                  /**</ Button pin instance. */
#define BUTTON_PIN               10                 /**</ Button pin number. */
#define BUTTON_PAD_ID            25                 /**</ Button Pad ID. */
#define BUTTON_PAD_ALT_FUNC      PAD_MuxAlt0        /**</ Button pin alternate function. */

//GPIO3 - LED
#define LED_INSTANCE             0                  /**</ LED pin instance. */
#define LED_GPIO_PIN             3                  /**</ LED pin number. */
#define LED_PAD_ID               14                 /**</ LED Pad ID. */
#define LED_PAD_ALT_FUNC         PAD_MuxAlt0        /**</ LED pin alternate function. */

#define LED_ON  1                                   /**</ LED on. */
#define LED_OFF 0                                   /**</ LED off. */

#define QUEUE_SIZE 10                               /**</ Queue size for LED commands. */

// Estrutura para comando do LED
typedef struct {
    uint8_t led_state;
    uint32_t timestamp;
} led_command_t;

// Handle da fila
QueueHandle_t led_queue;

static void HT_GPIO_InitButton(void) {
    GPIO_InitType GPIO_InitStruct = {0};

    GPIO_InitStruct.af = PAD_MuxAlt0;
    GPIO_InitStruct.pad_id = BUTTON_PAD_ID;
    GPIO_InitStruct.gpio_pin = BUTTON_PIN;
    GPIO_InitStruct.pin_direction = GPIO_DirectionInput;
    GPIO_InitStruct.pull = PAD_InternalPullUp;
    GPIO_InitStruct.instance = BUTTON_INSTANCE;
    GPIO_InitStruct.exti = GPIO_EXTI_DISABLED;
    GPIO_InitStruct.interrupt_config = GPIO_InterruptFallingEdge;

    HT_GPIO_Init(&GPIO_InitStruct);
}

static void HT_GPIO_InitLed(void) {
    GPIO_InitType GPIO_InitStruct = {0};

    GPIO_InitStruct.af = PAD_MuxAlt0;
    GPIO_InitStruct.pad_id = LED_PAD_ID;
    GPIO_InitStruct.gpio_pin = LED_GPIO_PIN;
    GPIO_InitStruct.pin_direction = GPIO_DirectionOutput;
    GPIO_InitStruct.init_output = 0;
    GPIO_InitStruct.pull = PAD_AutoPull;
    GPIO_InitStruct.instance = LED_INSTANCE;
    GPIO_InitStruct.exti = GPIO_EXTI_DISABLED;

    HT_GPIO_Init(&GPIO_InitStruct);
}

/**
 * @brief Task responsável por ler o estado do botão e enviar comandos para a fila
 */
void ButtonTask(void *pvParameters) {
    bool current_button_state = true;  // Inicializa com pull-up (não pressionado)
    bool previous_button_state = true;
    led_command_t led_cmd;
    
    printf("ButtonTask iniciada\n");
    
    while (1) {
        // Lê o estado atual do botão (invertido devido ao pull-up)
        current_button_state = !((bool) HT_GPIO_PinRead(BUTTON_INSTANCE, BUTTON_PIN));
        
        // Detecta mudança de estado (borda de subida ou descida)
        if (current_button_state != previous_button_state) {
            // Prepara comando para enviar à fila
            led_cmd.led_state = current_button_state ? LED_ON : LED_OFF;
            led_cmd.timestamp = xTaskGetTickCount();
            
            // Envia comando para a fila (sem bloquear)
            if (xQueueSend(led_queue, &led_cmd, 0) == pdTRUE) {
                printf("Comando enviado: LED %s (tick: %lu)\n", 
                       led_cmd.led_state ? "ON" : "OFF", led_cmd.timestamp);
            } else {
                printf("Erro: Fila cheia!\n");
            }
            
            previous_button_state = current_button_state;
        }
        
        vTaskDelay(pdMS_TO_TICKS(50)); // Debounce e polling a cada 50ms
    }
}

/**
 * @brief Task responsável por processar comandos da fila e controlar o LED
 */
void LedTask(void *pvParameters) {
    led_command_t received_cmd;
    
    printf("LedTask iniciada\n");
    
    while (1) {
        // Aguarda comando da fila (bloqueia indefinidamente)
        if (xQueueReceive(led_queue, &received_cmd, portMAX_DELAY) == pdTRUE) {
            // Executa o comando recebido
            HT_GPIO_WritePin(LED_GPIO_PIN, LED_INSTANCE, received_cmd.led_state);
            
            printf("LED %s executado (tick recebido: %lu, tick atual: %lu)\n", 
                   received_cmd.led_state ? "ON" : "OFF", 
                   received_cmd.timestamp, 
                   xTaskGetTickCount());
        }
    }
}

/**
 * @brief Task de monitoramento da fila (opcional)
 */
void MonitorTask(void *pvParameters) {
    UBaseType_t queue_items;
    
    while (1) {
        queue_items = uxQueueMessagesWaiting(led_queue);
        
        if (queue_items > 0) {
            printf("Fila: %u comandos aguardando processamento\n", queue_items);
        }
        
        vTaskDelay(pdMS_TO_TICKS(2000)); // Monitora a cada 2 segundos
    }
}

/**
  \fn          int main_entry(void)
  \brief       main entry function.
  \return
*/
void main_entry(void) {
    // Inicializa hardware
    HT_GPIO_InitButton();
    HT_GPIO_InitLed();
    slpManNormalIOVoltSet(IOVOLT_3_30V);

    HAL_USART_InitPrint(&huart1, GPR_UART1ClkSel_26M, uart_cntrl, 115200);
    printf("=== Exemplo FreeRTOS - Controle LED com Fila ===\n");

    // Cria a fila para comandos do LED
    led_queue = xQueueCreate(QUEUE_SIZE, sizeof(led_command_t));
    
    if (led_queue == NULL) {
        printf("Erro: Falha ao criar fila!\n");
        while(1); // Para execução em caso de erro
    }
    
    printf("Fila criada com sucesso (tamanho: %d)\n", QUEUE_SIZE);

    // Cria as tasks
    if (xTaskCreate(ButtonTask, "ButtonTask", 256, NULL, 2, NULL) != pdPASS) {
        printf("Erro: Falha ao criar ButtonTask!\n");
        while(1);
    }
    
    if (xTaskCreate(LedTask, "LedTask", 256, NULL, 2, NULL) != pdPASS) {
        printf("Erro: Falha ao criar LedTask!\n");
        while(1);
    }
    
    if (xTaskCreate(MonitorTask, "MonitorTask", 256, NULL, 1, NULL) != pdPASS) {
        printf("Erro: Falha ao criar MonitorTask!\n");
        while(1);
    }
    
    printf("Tasks criadas com sucesso\n");
    printf("Pressione o botão para controlar o LED\n");

    // Inicia o scheduler do FreeRTOS
    vTaskStartScheduler();
    
    printf("Erro: Scheduler não deveria retornar!\n");

    while(1);
}

/******** HT Micron Semicondutores S.A **END OF FILE*/
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

#include "HT_Sleep.h"
#include "slpman_qcx212.h"
#include "htnb32lxxx_hal_usart.h"
#include "debug_log.h"
#include "ps_lib_api.h"

// Callbacks para modo de sono
static void beforeHibernateCb(void *pdata, slpManLpState state) {
    printf("[Callback] Preparando para hibernacao\n");
}

static void afterHibernateCb(void *pdata, slpManLpState state) {
    printf("[Callback] Sistema acordou da hibernacao\n");
}

void HT_Sleep_EnterSleep(slpManSlpState_t state, uint32_t sleep_ms) {
    static uint8_t voteHandle = 0xFF;
    
    // Garante que todas as mensagens de log pendentes sejam enviadas antes de dormir
    uniLogFlushOut(0);
    
    printf("\n=== ENTRANDO EM MODO SONO %d POR %lu ms ===\n", state, sleep_ms);
    
    // Desativa funcoes de celular e SIM para economizar energia
    appSetCFUN(0);
    appSetEcSIMSleepSync(1);
    
    // Primeiro, configura o modo de sono
    // Este comando e importante para inicializar o sistema para o sono
    slpManSetPmuSleepMode(true, state, false);
    
    // Configura o gerenciamento de sleep
    if (voteHandle == 0xFF) {
        slpManApplyPlatVoteHandle("SLEEP_TEST", &voteHandle);
    }
    
    // Registra callbacks para hibernacao
    slpManRegisterUsrdefinedBackupCb(beforeHibernateCb, NULL, SLPMAN_HIBERNATE_STATE);
    slpManRegisterUsrdefinedRestoreCb(afterHibernateCb, NULL, SLPMAN_HIBERNATE_STATE);
    
    // Habilita o modo de sono
    slpManPlatVoteEnableSleep(voteHandle, state);
    
    // Configura o timer de sono como fonte de wakeup
    slpManDeepSlpTimerStart(DEEPSLP_TIMER_ID7, sleep_ms);
    
    // Espera passiva - o sistema deve entrar em sono automaticamente
    // Esta e a abordagem usada no exemplo de referencia
    while (1) {
        printf("Hibernando...\n");
        osDelay(2000);  // apos o tempo, sistema acorda e mensagens sao exibidas
    }
    
    // O codigo abaixo nunca sera executado
}
/************************ HT Micron Semicondutores S.A *****END OF FILE****/

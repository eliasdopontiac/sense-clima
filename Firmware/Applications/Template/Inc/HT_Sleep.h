/*

  _    _ _______   __  __ _____ _____ _____   ____  _   _
 | |  | |__   __| |  \/  |_   _/ ____|  __ \ / __ \| \ | |
 | |__| |  | |    | \  / | | || |    | |__) | |  | |  \| |
 |  __  |  | |    | |\/| | | || |    |  _  /| |  | | . ` |
 | |  | |  | |    | |  | |_| || |____| | \ \| |__| | |\  |
 |_|  |_|  |_|    |_|  |_|_____\_____|_|  \_\\____/|_| \_|
 =================== Advanced R&D ========================

 Copyright (c) 2023 HT Micron Semicondutores S.A.
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 http://www.apache.org/licenses/LICENSE-2.0
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

*/

/*!
 * \file HT_BSP_Custom.h
 * \brief Custom settings for iMCP HTNB32L-XXX. 
  * \author HT Micron Advanced R&D,
 *         Hêndrick Bataglin Gonçalves, Christian Roberto Lehmen,  Matheus da Silva Zorzeto, Felipe Kalinski Ferreira,
 *         Leandro Borges, Mauricio Carlotto Ribeiro, Henrique Kuhn, Cleber Haack, Eduardo Mendel
 *         Gleiser Alvarez Arrojo
 * 
 * \link https://github.com/htmicron
 * \version 0.1
 * \date March 15, 2023
 */

#ifndef __HT_SLEEP_H__
#define __HT_SLEEP_H__

#include "main.h"
#include "slpman_qcx212.h"
#include "pmu_qcx212.h"

// Timer ID para o deep sleep
#define DEEPSLP_TIMER_ID7 7

/*!******************************************************************
 * \fn void HT_Sleep_EnterSleep(slpManSlpState_t state, uint32_t sleep_ms)
 * \brief Enters a specified sleep state for a certain duration.
 *
 * This function configures the AON timer as a wakeup source and
 * then enters the specified sleep mode. Execution resumes after
 * the timer expires.
 *
 * \param[in]  state      The sleep state to enter (e.g., SLP_SLP1_STATE).
 * \param[in]  sleep_ms   Duration to sleep in milliseconds.
 *
 * \retval none
 *******************************************************************/
void HT_Sleep_EnterSleep(slpManSlpState_t state, uint32_t sleep_ms);

#endif /*__HT_SLEEP_H__*/

/************************ HT Micron Semicondutores S.A *****END OF FILE****/

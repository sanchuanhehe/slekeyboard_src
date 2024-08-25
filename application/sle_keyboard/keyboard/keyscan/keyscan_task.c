/**
 * Copyright (c) @CompanyNameMagicTag 2023-2023. All rights reserved. \n
 *
 * Description: KEYSCAN Sample Source. \n
 * Author: @CompanyNameTag \n
 * History: \n
 * 2023-07-26, Create file. \n
 */
#include "keyscan_task.h"
#include "app_init.h"
#include "cmsis_os2.h"
#include "common_def.h"
#include "keyscan.h"
#include "keyscan_porting.h"
#include "osal_debug.h"
#include "pinctrl.h"
#include "soc_osal.h"

#ifndef debug
#define debug
#endif

#ifdef debug
#include "debug_print.h"
#include "log_common.h"
#include "los_memory.h"
#endif
LOS_MEM_POOL_STATUS status;
static uint32_t g_at_uart_recv_cnt = 0;
static int app_keyscan_report_callback(int key_nums, uint8_t key_values[]) {
  osal_printk("key_nums = %d, key_values = ", key_nums);
  for (int i = 0; i < key_nums; i++) {
    osal_printk("%x ", key_values[i]);
  }
  osal_printk("\r\n");
#ifdef debug
  LOS_MemInfoGet(m_aucSysMem0, &status);
  PRINT(
      "[SYS INFO] mem: used:%u, free:%u; log: drop/all[%u/%u], at_recv %u.\r\n",
      status.uwTotalUsedSize, status.uwTotalFreeSize,
      log_get_missed_messages_count(), log_get_all_messages_count(),
      g_at_uart_recv_cnt);
#endif
  return 0;
}

void *keyscan_task(const char *arg) {
  uint8_t g_app_key_map[KEYSCAN_MAX_ROW][KEYSCAN_MAX_COL] = KEYSCAN_MAP;
  unused(arg);
  // keyscan_porting_config_pins();
  /* Set the key value matrix of Keyscan. */
  pin_t gpio_map[KEYSCAN_GPIO_NUM] = {2,  3,  4,  5,  6,  11, 12, 0,
                                      13, 14, 15, 16, 17, 18, 22, 23,
                                      24, 25, 26, 27, 0,  0,  0,  0};

  keyscan_porting_config_pins(gpio_map, KEYSCAN_GPIO_NUM);
  errcode_t ret;
  ret = uapi_set_keyscan_value_map((uint8_t **)g_app_key_map, KEYSCAN_MAX_ROW,
                                   KEYSCAN_MAX_COL);
  if (ret == ERRCODE_SUCC) {
    osal_printk("the key value matrix of Keyscan has been successfully "
                "configured!\r\n");
  }
  keyscan_porting_type_sel(CUSTOME_KEYS_TYPE);
#define KEYSCAN_PULSE_TIME 3
#define KEYSCAN_INTERRUPT_TYPE 3
  /* KEYSCAN Init Config. */
  uapi_keyscan_init(KEYSCAN_PULSE_TIME, HAL_KEYSCAN_MODE_0,
                    KEYSCAN_INTERRUPT_TYPE);

  osal_printk("keyscan register callback start!\r\n");
  ret = uapi_keyscan_register_callback(app_keyscan_report_callback);
  if (ret == ERRCODE_SUCC) {
    osal_printk("keyscan register callback succ!\r\n");
  }
  osal_printk("keyscan enable start!\r\n");
  ret = uapi_keyscan_enable();
  if (ret == ERRCODE_KEYSCAN_POWER_ON) {
    osal_printk("keyscan enable start succ!\r\n");
  }
  return NULL;
}
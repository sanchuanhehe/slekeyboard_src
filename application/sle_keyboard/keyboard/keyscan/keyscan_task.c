/**
 * Copyright (c) @CompanyNameMagicTag 2023-2023. All rights reserved. \n
 *
 * Description: KEYSCAN Sample Source. \n
 * Author: @CompanyNameTag \n
 * History: \n
 * 2023-07-26, Create file. \n
 */
#include "app_init.h"
#include "cmsis_os2.h"
#include "common_def.h"
#include "keyscan.h"
#include "keyscan_porting.h"
#include "osal_debug.h"
#include "pinctrl.h"
#include "soc_osal.h"

uint8_t g_app_key_map[KEYSCAN_MAX_ROW][KEYSCAN_MAX_COL] = {
    {0x29, 0x2B, 0x14, 0x35, 0x04, 0x1E, 0x1D, 0x00},
    {0x3D, 0x3C, 0x08, 0x3B, 0x07, 0x20, 0x06, 0x00},
    {0x00, 0x39, 0x1A, 0x3A, 0x16, 0x1F, 0x1B, 0x00},
    {0x00, 0x00, 0x00, 0xE0, 0x00, 0x00, 0xE4, 0x00},
    {0x0A, 0x17, 0x15, 0x22, 0x09, 0x21, 0x19, 0x05},
    {0x0B, 0x1C, 0x18, 0x23, 0x0D, 0x24, 0x10, 0x11},
    {0x3F, 0x30, 0x0C, 0x2E, 0x0E, 0x25, 0x36, 0x00},
    {0x00, 0x00, 0x12, 0x40, 0x0F, 0x26, 0x37, 0x00},
    {0x34, 0x2F, 0x13, 0x2D, 0x33, 0x27, 0x00, 0x38},
    {0x3E, 0x2A, 0x00, 0x41, 0x31, 0x42, 0x28, 0x2C},
    {0x00, 0x00, 0xE3, 0x00, 0x00, 0x43, 0x00, 0x51},
    {0xE2, 0x00, 0x00, 0x00, 0x00, 0x45, 0x2C, 0xE6},
    {0x00, 0x53, 0x00, 0x00, 0xE1, 0x44, 0x00, 0x4F},
    {0x52, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4C, 0x50},
    {0x5F, 0x5C, 0x61, 0x5E, 0x59, 0x62, 0x55, 0x5B},
    {0x54, 0x60, 0x56, 0x57, 0x5D, 0x5A, 0x58, 0x63}};

static int app_keyscan_report_callback(int key_nums, uint8_t key_values[]) {
  osal_printk("key_nums = %d, key_values = ", key_nums);
  for (int i = 0; i < key_nums; i++) {
    osal_printk("%x ", key_values[i]);
  }
  return 0;
}

void *keyscan_task(const char *arg) {
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
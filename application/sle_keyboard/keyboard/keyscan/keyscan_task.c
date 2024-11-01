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
#include "gadget/f_hid.h"
#include "keyscan.h"
#include "keyscan_porting.h"
#include "osal_debug.h"
#include "pinctrl.h"
#include "sle_device_discovery.h"
#include "sle_errcode.h"
#include "sle_keyboard_server.h"
#include "sle_keyboard_server_adv.h"
#include "soc_osal.h"
#include "usb_init_keyboard_app.h"

#ifndef debug
#define debug
#endif

#ifdef debug
#include "debug_print.h"
#include "log_common.h"
#include "los_memory.h"
#endif
#define USB_HID_KEYBOARD_MAX_KEY_LENTH 6
#define USB_KEYBOARD_REPORTER_LEN 9
#define KEYSCAN_REPORT_MAX_KEY_NUMS 6
#define CONVERT_DEC_TO_HEX 16
#define MAX_NUM_OF_DEC 10
#define LENGTH_OF_KEY_VALUE_STR 5
#define USB_HID_SPECIAL_KEY_MIN 0xE0
#define USB_HID_SPECIAL_KEY_MAX 0xE7
#define SLE_KEYBOARD_PARAM_ARGC_2 2
#define SLE_KEYBOARD_PARAM_ARGC_3 3
#define SLE_KEYBOARD_PARAM_ARGC_4 4
#define SLE_KEYBOARD_SERVER_MSG_QUEUE_LEN 5
#define SLE_KEYBOARD_SERVER_MSG_QUEUE_MAX_SIZE 32
#define SLE_KEYBOARD_SERVER_QUEUE_DELAY 0xFFFFFFFF
#define SLE_ADV_HANDLE_DEFAULT 1

#ifndef ENANLE_USB_HID_KEYBOARD
#define ENANLE_USB_HID_KEYBOARD
#endif

typedef struct usb_hid_keyboard_report {
  uint8_t kind;
  uint8_t special_key; /*!< 8bit special key(Lctrl Lshift Lalt Lgui Rctrl Rshift
                          Ralt Rgui) */
  uint8_t reversed;    /*!< Reversed, Must be zero */
  uint8_t key[USB_HID_KEYBOARD_MAX_KEY_LENTH]; /*!< Normal key */
} usb_hid_keyboard_report_t;

static usb_hid_keyboard_report_t g_hid_keyboard_report;
static unsigned long g_sle_keyboard_server_msgqueue_id;

static void ssaps_server_read_request_cbk(uint8_t server_id, uint16_t conn_id,
                                          ssaps_req_read_cb_t *read_cb_para,
                                          errcode_t status) {
  if (read_cb_para == NULL) {
    osal_printk(
        "%s ssaps_server_read_request_cbk fail, read_cb_para is null!\r\n",
        SLE_KEYBOARD_SERVER_LOG);
    return;
  }
  osal_printk("%s ssaps read request cbk callback server_id:%x, conn_id:%x, "
              "handle:%x, status:%x\r\n",
              SLE_KEYBOARD_SERVER_LOG, server_id, conn_id, read_cb_para->handle,
              status);
}
static void ssaps_server_write_request_cbk(uint8_t server_id, uint16_t conn_id,
                                           ssaps_req_write_cb_t *write_cb_para,
                                           errcode_t status) {
  unused(server_id);
  unused(conn_id);
  char *key_value_str[KEYSCAN_REPORT_MAX_KEY_NUMS];
  uint8_t *key_values = NULL;
  uint16_t key_nums, i;

  if (write_cb_para == NULL) {
    osal_printk(
        "%s ssaps_server_write_request_cbk fail, write_cb_para is null!\r\n",
        SLE_KEYBOARD_SERVER_LOG);
    return;
  }
  osal_printk("%s ssaps write request callback cbk handle:%x, status:%x\r\n",
              SLE_KEYBOARD_SERVER_LOG, write_cb_para->handle, status);
  if ((write_cb_para->length > 0) && write_cb_para->value) {
    // todo,recv data from client,send to pc by uart or send to keyboard
    osal_printk("%s recv data from client, len =[%d], data= ",
                SLE_KEYBOARD_SERVER_LOG, write_cb_para->length);
    for (i = 0; i < write_cb_para->length; i++) {
      osal_printk("0x%02x ", write_cb_para->value[i]);
    }
    osal_printk("\r\n");
    key_values = write_cb_para->value;
    key_nums = write_cb_para->length;
    for (i = 0; i < write_cb_para->length; i++) {
      key_value_str[i] = (char *)osal_vmalloc(LENGTH_OF_KEY_VALUE_STR);
      if (key_value_str[i] == NULL) {
        osal_printk("[ERROR]send input report new fail\r\n");
        return;
      }
      key_value_str[i][0] = '0';
      key_value_str[i][1] = 'x';
      uint32_t tran = key_values[i] / CONVERT_DEC_TO_HEX;
      if (tran < MAX_NUM_OF_DEC) {
        key_value_str[i][SLE_KEYBOARD_PARAM_ARGC_2] = '0' + tran;
      } else {
        key_value_str[i][SLE_KEYBOARD_PARAM_ARGC_2] =
            ('A' + tran - MAX_NUM_OF_DEC);
      }
      tran = key_values[i] % CONVERT_DEC_TO_HEX;
      if (tran < MAX_NUM_OF_DEC) {
        key_value_str[i][SLE_KEYBOARD_PARAM_ARGC_3] = '0' + tran;
      } else {
        key_value_str[i][SLE_KEYBOARD_PARAM_ARGC_3] =
            ('A' + tran - MAX_NUM_OF_DEC);
      }
      key_value_str[i][SLE_KEYBOARD_PARAM_ARGC_4] = '\0';
    }
    // 发送到键盘
    if (key_nums > 0) {
      uapi_ble_hid_keyboard_input_str(key_nums, (char **)key_value_str);
    }
    for (i = 0; i < key_nums; i++) {
      osal_vfree(key_value_str[i]);
    }
  }
}

static void sle_keyboard_server_create_msgqueue(void) {
  if (osal_msg_queue_create("sle_keyboard_server_msgqueue",
                            SLE_KEYBOARD_SERVER_MSG_QUEUE_LEN,
                            (unsigned long *)&g_sle_keyboard_server_msgqueue_id,
                            0, SLE_KEYBOARD_SERVER_MSG_QUEUE_MAX_SIZE) != EOK) {
    osal_printk("^%s sle_keyboard_server_create_msgqueue message queue create "
                "failed!\n",
                SLE_KEYBOARD_SERVER_LOG);
  }
}

static void sle_keyboard_server_delete_msgqueue(void) {
  osal_msg_queue_delete(g_sle_keyboard_server_msgqueue_id);
}

static void sle_keyboard_server_write_msgqueue(uint8_t *buffer_addr,
                                               uint32_t buffer_size) {
  if (buffer_addr == NULL) {
    osal_printk(
        "%s sle_keyboard_server_write_msgqueue fail, buffer_addr is null!\r\n",
        SLE_KEYBOARD_SERVER_LOG);
    return;
  }
  osal_msg_queue_write_copy(g_sle_keyboard_server_msgqueue_id,
                            (void *)buffer_addr, (uint32_t)buffer_size, 0);
}

static int32_t sle_keyboard_server_receive_msgqueue(uint8_t *buffer_addr,
                                                    uint32_t *buffer_size) {
  if (buffer_addr == NULL) {
    osal_printk("%s sle_keyboard_server_receive_msgqueue fail, buffer_addr is "
                "null!\r\n",
                SLE_KEYBOARD_SERVER_LOG);
    return -1;
  }
  return osal_msg_queue_read_copy(g_sle_keyboard_server_msgqueue_id,
                                  (void *)buffer_addr, buffer_size,
                                  SLE_KEYBOARD_SERVER_QUEUE_DELAY);
}

static void sle_keyboard_server_rx_buf_init(uint8_t *buffer_addr,
                                            uint32_t buffer_size) {
  if (buffer_addr == NULL) {
    osal_printk(
        "%s sle_keyboard_server_rx_buf_init fail, buffer_addr is null!\r\n",
        SLE_KEYBOARD_SERVER_LOG);
    return;
  }
  if (memset_s(buffer_addr, buffer_size, 0, buffer_size) != EOK) {
    osal_printk("%s sle_keyboard_server_rx_buf_init memset_s fail!\r\n",
                SLE_KEYBOARD_SERVER_LOG);
  }
}

LOS_MEM_POOL_STATUS status;
static uint32_t g_at_uart_recv_cnt = 0;

#ifdef ENANLE_USB_HID_KEYBOARD
static int g_usb_keyscan_hid_index = -1;
static void usb_keyscan_send_data(usb_hid_keyboard_report_t *rpt) {
  rpt->kind = 0x1;

  int32_t ret = fhid_send_data(g_usb_keyscan_hid_index, (char *)rpt,
                               USB_KEYBOARD_REPORTER_LEN);
  if (ret == -1) {
    osal_printk("send data falied! ret:%d\n", ret);
    return;
  }
}

#endif

static int app_keyscan_report_callback(int key_nums, uint8_t key_values[]) {
#ifdef debug
  osal_printk("key_nums = %d, key_values = ", key_nums);
  for (int i = 0; i < key_nums; i++) {
    osal_printk("%x ", key_values[i]);
  }
  osal_printk("\r\n");
  LOS_MemInfoGet(m_aucSysMem0, &status);
  PRINT(
      "[SYS INFO] mem: used:%u, free:%u; log: drop/all[%u/%u], at_recv %u.\r\n",
      status.uwTotalUsedSize, status.uwTotalFreeSize,
      log_get_missed_messages_count(), log_get_all_messages_count(),
      g_at_uart_recv_cnt);
#endif
  uint8_t normal_key_num = 0;
  if (memset_s(&g_hid_keyboard_report, sizeof(g_hid_keyboard_report), 0,
               sizeof(g_hid_keyboard_report)) != EOK) {
    return 0;
  }
  for (uint8_t i = 0; i < key_nums; i++) {
    uint8_t tmp_key = key_values[i];
    if (tmp_key >= USB_HID_SPECIAL_KEY_MIN &&
        tmp_key <= USB_HID_SPECIAL_KEY_MAX) {
      g_hid_keyboard_report.special_key |=
          (1 << (tmp_key - USB_HID_SPECIAL_KEY_MIN));
    } else {
      g_hid_keyboard_report.key[normal_key_num] = tmp_key;
      normal_key_num++;
    }
  }
  g_hid_keyboard_report.kind = 0x01;
  if (sle_keyboard_client_is_connected() == 0) {
#ifdef debug
    osal_printk("sle_keyboard_client_is_connected fail!\r\n");
#endif
#ifdef ENANLE_USB_HID_KEYBOARD
    usb_keyscan_send_data(&g_hid_keyboard_report);
#endif
    return 1;
  } else {
    sle_keyboard_server_send_report_by_handle(
        (uint8_t *)(uintptr_t)&g_hid_keyboard_report,
        sizeof(usb_hid_keyboard_report_t));
  }
  return 1;
}

void *keyscan_task(const char *arg) {
  unused(arg);
  osal_printk("enter sle_keyboard_task!\r\n");
#ifdef ENANLE_USB_HID_KEYBOARD
  int index = usb_init_keyboard_app(DEV_HID);
  if (index < 0) {
    osal_printk("usb_init_keyboard_app fail!\r\n");
    return NULL;
  }
  g_usb_keyscan_hid_index = index;
#endif
  errcode_t ret;
#define SLE_KEYBOARD_SERVER_MSG_QUEUE_LEN 5
#define SLE_KEYBOARD_SERVER_MSG_QUEUE_MAX_SIZE 32
  // 1. create msg queue
  uint8_t rx_buf[SLE_KEYBOARD_SERVER_MSG_QUEUE_MAX_SIZE] = {0};
  uint32_t rx_length = SLE_KEYBOARD_SERVER_MSG_QUEUE_MAX_SIZE;
  uint8_t sle_connect_state[] = "sle_dis_connect";
  // 2.delay for sle start
#define SLE_KEYBOARD_TASK_DURATION_MS 2000
#define SLE_KEYBOARD_SERVER_DELAY_COUNT 3
  osDelay(SLE_KEYBOARD_TASK_DURATION_MS * SLE_KEYBOARD_SERVER_DELAY_COUNT);

  // 3.keyscan init enable config
  uint8_t g_app_key_map[KEYSCAN_MAX_ROW][KEYSCAN_MAX_COL] = KEYSCAN_MAP;
  unused(arg);
  // keyscan_porting_config_pins();
  /* Set the key value matrix of Keyscan. */
  pin_t gpio_map[KEYSCAN_GPIO_NUM] = {2,  3,  4,  5,  6,  11, 12, 0,
                                      13, 14, 15, 16, 17, 18, 22, 23,
                                      24, 25, 26, 27, 0,  0,  0,  0};

  keyscan_porting_config_pins(gpio_map, KEYSCAN_GPIO_NUM);

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
  // 4.sle server init.
  sle_keyboard_server_create_msgqueue();
  sle_keyboard_server_register_msg(sle_keyboard_server_write_msgqueue);
  sle_keyboard_server_init(ssaps_server_read_request_cbk,
                           ssaps_server_write_request_cbk);
  while (1) {
    sle_keyboard_server_rx_buf_init(rx_buf, rx_length);
    sle_keyboard_server_receive_msgqueue(rx_buf, &rx_length);
    if (strncmp((const char *)rx_buf, (const char *)sle_connect_state,
                sizeof(sle_connect_state)) == 0) {
      ret = sle_start_announce(SLE_ADV_HANDLE_DEFAULT);
      if (ret != ERRCODE_SLE_SUCCESS) {
        osal_printk("%s sle_connect_state_changed_cbk,sle_start_announce fail "
                    ":%02x\r\n",
                    SLE_KEYBOARD_SERVER_LOG, ret);
      }
    }
    if (sle_keyboard_client_is_connected() == 0) {
      osal_printk("sle_keyboard_client_is_connected fail!\r\n");
    }
    rx_length = SLE_KEYBOARD_SERVER_MSG_QUEUE_MAX_SIZE;
  }
#ifdef ENANLE_USB_HID_KEYBOARD
  usb_deinit_keyscan_app();
#endif
  sle_keyboard_server_delete_msgqueue();
  uapi_keyscan_deinit();
  return NULL;
}
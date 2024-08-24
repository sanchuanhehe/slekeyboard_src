/**
 * Copyright (c) @CompanyNameMagicTag 2023-2023. All rights reserved. \n
 *
 * Description: SPI Sample Source. \n
 * Author: @CompanyNameTag \n
 * History: \n
 * 2023-06-25, Create file. \n
 */
#include "app_init.h"
#include "cmsis_os2.h"
#include "dma.h"
#include "osal_debug.h"
#include "pinctrl.h"
#include "soc_osal.h"
#include "spi.h"

#ifndef debug
#define debug
#endif

#ifndef dma_enable
// #define dma_enable
#endif

#define SPI_SLAVE_NUM 1
#define SPI_FREQUENCY 3
#define SPI_CLK_POLARITY 0
#define SPI_CLK_PHASE 0
#define SPI_FRAME_FORMAT 0
#define SPI_FRAME_FORMAT_STANDARD 0
#define SPI_FRAME_SIZE_8 0x1f
#define SPI_TMOD 0
#define SPI_WAIT_CYCLES 0x10

#define SPI_TASK_STACK_SIZE 0x2000
#define SPI_TASK_DURATION_MS 500
#define SPI_TASK_PRIO OSAL_TASK_PRIORITY_HIGH

#define LED_COUNT 1
#define LED_DATE_LEN 3 ///< 一个 LED 的数据长度（即3个字节，GRB顺序）
#define SPI_TRANSFER_LEN                                                       \
  ((LED_DATE_LEN * 4) * LED_COUNT) ///< 一个 LED 的数据经过编码后的长度

typedef struct {
  uint8_t green;
  uint8_t red;
  uint8_t blue;
} grb_t;

grb_t original_led_data = {0x01, 0x00,
                           0x00}; // 例子：绿色最大亮度，红色和蓝色为0

/**
 * @brief 将原始数据编码为 LED 驱动器可识别的数据
 *
 * @param input_data 原始数据 @ref grb_t
 * @param output_data 编码后的数据 (每个字节包含两个 4-bit 编码信息)
 * @param length  数据长度 (LED 数量) @ref LED_COUNT
 */
void encode_led_data(grb_t *input_data, uint8_t *output_data, uint32_t length) {
  for (uint32_t i = 0; i < length; i++) {
    uint8_t *byte_ptr = (uint8_t *)(&input_data[i]);
    for (uint8_t color = 0; color < LED_DATE_LEN; color++) {
      uint8_t byte = byte_ptr[color];
      for (uint8_t bit_pair = 0; bit_pair < 4; bit_pair++) {
        uint8_t bit_val_1 = (byte >> (7 - 2 * bit_pair)) & 0x01; // 取出高位
        uint8_t bit_val_2 = (byte >> (6 - 2 * bit_pair)) & 0x01; // 取出低位

        uint8_t encoded_val_1 = (bit_val_1 == 0) ? 0b1000 : 0b1100;
        uint8_t encoded_val_2 = (bit_val_2 == 0) ? 0b1000 : 0b1100;

        // 将两个 4-bit 编码信息组合到一个字节
        output_data[i * LED_DATE_LEN * 4 + color * 4 + bit_pair] =
            (encoded_val_1 << 4) | encoded_val_2;
      }
    }
  }
#ifdef debug
  osal_printk("encode_led_data: ");
  for (uint32_t i = 0; i < length * LED_DATE_LEN * 4; i++) {
    osal_printk("0x%02x ", output_data[i]);
  }
  osal_printk("\r\n");
#endif
}

static void app_spi_init_pin(void) {
  errcode_t ret = uapi_pin_set_mode(S_MGPIO0, HAL_PIO_SPI0_TXD);
  if (ret != ERRCODE_SUCC) {
    osal_printk("set pin mode failed .\n");
  }
}

static inline void __attribute__((always_inline))
app_spi_master_init_config(void) {
#ifdef debug
  osal_printk("spi%d master init start!\r\n", SPI_BUS_0); // TODO:debug
#endif
  spi_attr_t config = {0};
  spi_extra_attr_t ext_config = {0};

  config.is_slave = false;
  config.slave_num = SPI_SLAVE_NUM;
  config.bus_clk = SPI_CLK_FREQ;
  config.freq_mhz = SPI_FREQUENCY;
  config.clk_polarity = SPI_CLK_POLARITY;
  config.clk_phase = SPI_CLK_PHASE;
  config.frame_format = SPI_FRAME_FORMAT;
  config.spi_frame_format = HAL_SPI_FRAME_FORMAT_STANDARD;
  config.frame_size = SPI_FRAME_SIZE_8;
  config.tmod = SPI_TMOD;
  config.sste = 1;

  ext_config.qspi_param.wait_cycles = SPI_WAIT_CYCLES;
  errcode_t ret;
#ifdef dma_enable
#ifdef debug
  osal_printk("spi%d master dma init start!\r\n", SPI_BUS_0); // TODO:debug
#endif
  ret = uapi_dma_init();
  if (ret != ERRCODE_SUCC) {
    osal_printk("uapi_dma_init failed .\n");
  }
#ifdef debug
  osal_printk("spi%d master dma init end!\r\n", SPI_BUS_0);   // TODO:debug
  osal_printk("spi%d master dma open start!\r\n", SPI_BUS_0); // TODO:debug
#endif
  ret = uapi_dma_open();
  if (ret != ERRCODE_SUCC) {
    osal_printk("uapi_dma_init failed .\n");
  }
#ifdef debug
  osal_printk("spi%d master dma open end!\r\n", SPI_BUS_0); // TODO:debug
#endif
#endif
  ret = uapi_spi_init(SPI_BUS_0, &config, &ext_config);
  if (ret != ERRCODE_SUCC) {
    osal_printk("uapi_spi_init failed .\n");
    osal_printk("errcode = %d\n", ret);
  } else {
    osal_printk("uapi_spi_init success .\n");
  }
#ifdef debug
  osal_printk("spi%d master init end!\r\n", SPI_BUS_0); // TODO:debug
#endif
}

static void *spi_master_task(const char *arg) {
  if(arg != NULL)
  {
    osal_printk("spi_master_task arg: %s\n", arg);
  }
  else
  {
    osal_printk("spi_master_task arg is NULL\n");
  }
  unused(arg);
  /* SPI pinmux. */
  app_spi_init_pin();

  /* SPI master init config. */
  app_spi_master_init_config();

  /* SPI data config. */
  uint8_t tx_data[SPI_TRANSFER_LEN] = {0};
  // for (uint32_t loop = 0; loop < SPI_TRANSFER_LEN; loop++) {
  //   tx_data[loop] = (loop & 0xFF);
  // }

  encode_led_data(&original_led_data, tx_data,
                  LED_COUNT); // 编码一个 LED 的数据

  spi_xfer_data_t data = {
      .tx_buff = tx_data,
      .tx_bytes = SPI_TRANSFER_LEN,
  };

  while (1) {
    osDelay(SPI_TASK_DURATION_MS);
    osal_printk("spi%d master send start!\r\n", SPI_BUS_0);
    if (uapi_spi_master_write(SPI_BUS_0, &data, 0xFFFFFFFF) == ERRCODE_SUCC) {
      osal_printk("spi%d master send succ!\r\n", SPI_BUS_0);
      return NULL;
    } else {
      continue;
    }
  }

  return NULL;
}

static void spi_master_entry(void) {
  int ret;
  osal_task *taskid;
  // 创建任务调度
  osal_kthread_lock();
  // 创建任务
  taskid = osal_kthread_create((osal_kthread_handler)spi_master_task, NULL,
                               "spi_master_task", SPI_TASK_STACK_SIZE);
  if (taskid == NULL) {
    osal_printk("create spi_master_task failed .\n");
    return;
  }
  ret = osal_kthread_set_priority(taskid, SPI_TASK_PRIO);
  if (ret != OSAL_SUCCESS) {
    osal_printk("set spi_master_task priority failed .\n");
  }
  osal_kthread_unlock();
}

/* Run the spi_master_entry. */
app_run(spi_master_entry);
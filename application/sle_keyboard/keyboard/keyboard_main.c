#include "app_init.h"
#include "gpio.h"
#include "keyscan_task.h"
#include "led.h"
#include "osal_task.h"
#include "pinctrl.h"
#include "platform_core.h"
#include "soc_osal.h"
#include "trng.h"

#ifndef debug
// #define debug
#endif

static void spi_master_entry(void) {
#define SPI_TASK_STACK_SIZE 0x800
#define SPI_TASK_PRIO OSAL_TASK_PRIORITY_HIGH
  int ret;
  osal_task *taskid;

  // 创建任务调度
  // osal_kthread_lock();

// 定义所需的led_data_t结构体的数量
#define LED_DATA_COUNT 6

  // 初始化LED数据
  grb_t led_data_0[10];
  grb_t led_data_1[14];
  grb_t led_data_28[14];
  grb_t led_data_29[15];
  grb_t led_data_30[15];
  grb_t led_data_31[16];

  grb_t *led_data_array[LED_DATA_COUNT] = {led_data_0,  led_data_1,
                                           led_data_28, led_data_29,
                                           led_data_30, led_data_31};
  uint8_t led_data_lengths[LED_DATA_COUNT] = {10, 14, 14, 15, 15, 16};
  pin_t led_data_pins[LED_DATA_COUNT] = {0, 1, 28, 29, 30, 31};

  // 缓冲区，用于存储生成的随机数
  uint8_t random_buffer[3];

  // 生成并填充随机数据
  for (int j = 0; j < LED_DATA_COUNT; j++) {
    for (int i = 0; i < led_data_lengths[j]; i++) {
      // 获取3字节的随机数，用于填充green, red, blue
      errcode_t ret = uapi_drv_cipher_trng_get_random_bytes(
          random_buffer, sizeof(random_buffer));
      if (ret == ERRCODE_SUCC) {
        // 将随机数填充到led_data数组中
        led_data_array[j][i].green = random_buffer[0] & 0b1111;
        led_data_array[j][i].red = random_buffer[1] & 0b1111;
        led_data_array[j][i].blue = random_buffer[2] & 0b1111;
      } else {
        // 处理错误情况，例如输出错误信息
        osal_printk("Failed to get random number for LED data, index: %d, "
                    "led_data_index: %d\n",
                    i, j);
      }
    }
  }

  // 创建6个led_data_t结构体并创建任务
  for (int j = 0; j < LED_DATA_COUNT; j++) {
    led_data_t led_data_config = {
        .led_data = led_data_array[j],
        .length = led_data_lengths[j],
        .pin = led_data_pins[j],
        .bus = SPI_BUS_0,
    };
    // 创建任务
    taskid = osal_kthread_create((osal_kthread_handler)spi_led_transfer_task,
                                 &led_data_config, "spi_master_task",
                                 SPI_TASK_STACK_SIZE);
    osal_msleep(10);///< 100ms延时, 防止任务创建失败
    if (taskid == NULL) {
      osal_printk("create spi_master_task %d failed .\n", j);
      return;
    } else {
      osal_printk("create spi_master_task %d success .\n", j);
      osal_kfree(taskid);
    }
    // ret = osal_kthread_set_priority(taskid, SPI_TASK_PRIO);
    // if (ret != OSAL_SUCCESS) {
    //   osal_printk("set spi_master_task %d priority failed .\n", j);
    //   osal_kfree(taskid);
    // } else {
    //   osal_kfree(taskid);
    // }
  }

#define KEYSCAN_TASK_STACK_SIZE 0x1000
  // 创建keyscan_task任务
  taskid = osal_kthread_create((osal_kthread_handler)keyscan_task, NULL,
                               "keyscan_task", KEYSCAN_TASK_STACK_SIZE);
  if (taskid == NULL) {
    osal_printk("create keyscan_task failed .\n");
    return;
  }
  ret = osal_kthread_set_priority(taskid, SPI_TASK_PRIO);
  if (ret != OSAL_SUCCESS) {
    osal_printk("set keyscan_task priority failed .\n");
    osal_kfree(taskid);
  } else {
    osal_kfree(taskid);
  }

  // osal_kthread_unlock();
}

/* Run the spi_master_entry. */
app_run(spi_master_entry);

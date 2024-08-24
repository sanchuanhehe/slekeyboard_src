#include "app_init.h"
#include "led.h"
#include "osal_task.h"
#include "platform_core.h"
#include "soc_osal.h"
#include "trng.h"

static void spi_master_entry(void) {
#define SPI_TASK_STACK_SIZE 0x2000
#define SPI_TASK_PRIO OSAL_TASK_PRIORITY_HIGH
  int ret;
  osal_task *taskid;
  // 创建任务调度
  osal_kthread_lock();
  // 初始化LED数据
  // 随机生成长度为10的grb_t数组
  grb_t led_data[10];

  // 缓冲区，用于存储生成的随机数
  uint8_t random_buffer[3];

  // 生成并填充随机数据
  for (int i = 0; i < 10; i++) {
    // 获取3字节的随机数，用于填充green, red, blue
    errcode_t ret = uapi_drv_cipher_trng_get_random_bytes(
        random_buffer, sizeof(random_buffer));
    if (ret == ERRCODE_SUCC) {
      // 将随机数填充到led_data数组中
      led_data[i].green = random_buffer[0];
      led_data[i].red = random_buffer[1];
      led_data[i].blue = random_buffer[2];
    } else {
      // 处理错误情况，例如输出错误信息
      osal_printk("Failed to get random number for LED data, index: %d\n", i);
      // 可以根据需求继续重试或者退出循环
    }
  }

  //   grb_t led_data = {0, 1, 0};
  led_data_t led_data_config = {
      .led_data = led_data,
      .length = 10,
      .pin = S_MGPIO0,
      .bus = SPI_BUS_0,
  };
  // 创建任务
  taskid = osal_kthread_create((osal_kthread_handler)spi_led_transfer_task,
                               &led_data_config, "spi_master_task",
                               SPI_TASK_STACK_SIZE);
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
#include "app_init.h"
#include "led.h"
#include "osal_task.h"
#include "platform_core.h"
#include "soc_osal.h"

static void spi_master_entry(void) {
#define SPI_TASK_STACK_SIZE 0x2000
#define SPI_TASK_PRIO OSAL_TASK_PRIORITY_HIGH
  int ret;
  osal_task *taskid;
  // 创建任务调度
  osal_kthread_lock();
  grb_t led_data = {0, 1, 0};
  led_data_t led_data_config = {
      .led_data = &led_data,
      .length = 1,
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
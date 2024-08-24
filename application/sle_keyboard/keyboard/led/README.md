# LED 驱动 SPI 通信任务

## 简介

该项目提供了一个通过 SPI 总线驱动 LED 的示例代码。主要包括一个用于初始化和配置 SPI 传输的任务函数，以及相应的数据结构定义。通过这个示例，用户可以了解如何使用 SPI 来控制 LED 灯的颜色。

## 文件说明

### 1. `led.h`

`led.h` 文件定义了用于 SPI 通信控制 LED 的数据结构和函数。具体内容包括：

- **grb_t 结构体**: 表示 LED 的颜色数据，包含三个字段分别表示绿色（`green`）、红色（`red`）、蓝色（`blue`）的值。
  
- **led_data_t 结构体**: 包含了 LED 的数据指针、LED 数量、使用的 SPI 引脚以及 SPI 总线。通过这个结构体，用户可以定义 LED 的配置信息并传递给任务函数。

- **`spi_led_transfer_task` 函数**: 该函数处理用于 LED 数据传输的 SPI 通信。它初始化 SPI 引脚和主机配置，编码 LED 数据，并通过 SPI 发送数据直到成功。

### 2. `spi_master_entry` 函数

`spi_master_entry` 函数演示了如何使用上述的 LED 数据结构和任务函数来创建和管理一个 SPI 任务。步骤如下：

- **初始化 LED 数据和配置**: 定义了一个 `grb_t` 结构体表示 LED 的颜色，并通过 `led_data_t` 结构体将其与 SPI 引脚和总线绑定。
  
- **创建和启动 SPI 任务**: 使用 `osal_kthread_create` 创建 SPI 通信任务，并设置任务的优先级。

### 3. `app_run(spi_master_entry)`

调用 `spi_master_entry` 以启动 SPI 任务调度。

## 如何使用

1. **导入头文件**: 在需要使用 SPI 驱动 LED 的代码中包含 `led.h`。

2. **初始化 LED 配置**: 根据你的需求设置 LED 的颜色数据、长度、SPI 引脚和总线。

3. **启动 SPI 任务**: 通过 `spi_master_entry` 函数或直接调用 `spi_led_transfer_task` 函数启动 SPI 数据传输任务。

## 注意事项

- **LED_DATE_LEN**: 定义了单个 LED 的数据长度为 3（GRB 顺序）。
- **SPI 引脚与总线**: 目前仅支持 SPI 引脚与 SPI 总线的通信。
- **错误处理**: 在创建任务和设置优先级时提供了基本的错误处理。

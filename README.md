# Diansaidacangku

送药小车项目代码仓库，包含 K230 端串口测试程序和 MSPM0 端循迹、姿态传感器及电机控制工程。

## 目录结构

- `k230/k230_mspm0_uart_test.py`：CanMV K230 与 MSPM0G3507 的 UART 通信测试程序。
- `mspm0/MSPM0_LineFollow_JY61P/`：Code Composer Studio（CCS）工程，包含循迹传感器、JY61P 姿态传感器、电机控制及 K230 UART 通信代码。

## K230 说明

目前 K230 端仅保留可直接运行的 Python 文件，具体 IDE/工作区配置尚未整理。程序面向 CanMV K230 环境，使用 `machine.FPIOA` 与 `machine.UART`。

## MSPM0 说明

MSPM0 工程面向 MSPM0G3507。仓库保留 CCS 工程配置与源代码，不包含 `Debug` 编译产物、本地缓存和个人工具配置。

## 串口连接

- K230 GPIO3（UART1_TX）→ MSPM0 PB3（UART3_RX）
- K230 GPIO4（UART1_RX）← MSPM0 PB2（UART3_TX）
- 两块开发板 GND 共地
- 串口参数：115200 baud、8N1

## 使用提示

1. 使用 CanMV 环境运行 K230 Python 文件。
2. 使用 Code Composer Studio 打开 MSPM0 工程目录。
3. 检查硬件连线与供电后，再分别下载并运行两端程序。

## 许可证

当前仓库暂未添加开源许可证。代码可公开查看，但复制、修改或再发布前请先联系仓库所有者。

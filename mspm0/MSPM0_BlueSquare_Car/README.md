# K230 蓝色方块控制小车

## 功能

- K230 连续识别到蓝色近似方块 3 帧后，小车以 50% 占空比前进。
- 连续丢失目标 3 帧后停车。
- MSPM0 连续 300 ms 收不到 K230 命令时强制停车。
- K230 退出或发生异常时尽力连续发送 3 次停车命令。

## 接线

| MSPM0G3507 | 连接目标 |
|---|---|
| PB3 | K230 GPIO3（UART1_TX） |
| PB2 | K230 GPIO4（UART1_RX，可选） |
| GND | K230 GND |
| PA21 | 驱动板 PWMC（左轮 Motor C） |
| PB6 | 驱动板 CIN1 |
| PB7 | 驱动板 CIN2 |
| PA22 | 驱动板 PWMB（右轮 Motor B） |
| PB8 | 驱动板 BIN1 |
| PB9 | 驱动板 BIN2 |

串口参数：115200 baud、8N1。

## 使用

1. CCS 选择 `File > Import Projects`。
2. 导入 `ticlang/blue_square_car_LP_MSPM0G3507_nortos_ticlang.projectspec`。
3. 工程使用 `SEGGER J-Link Emulator`，编译并下载 MSPM0 程序。
4. CanMV IDE 打开 `k230/k230_blue_square_car.py` 并运行。
5. Preview 左上角显示红色 `STOP` 或绿色 `FORWARD`；目标通过判断后显示绿色框。
6. 首次测试时抬起车轮，确认左右轮方向正确后再落地。

## 调试

- 某侧车轮方向相反时，将 `main.c` 中对应的 `LEFT_FORWARD_IN1_HIGH` 或 `RIGHT_FORWARD_IN1_HIGH` 改为 `0`。
- 现场光照导致识别异常时，使用 CanMV IDE Threshold Editor 调整 `BLUE_THRESHOLDS`。
- K230 控制台的 `found=True` 表示识别成功，`command=DRIVE,1` 表示已发送前进指令。
- K230 离线运行时，将脚本保存为 `/sdcard/main.py`。

> 首次运行务必让车轮离地，并确保 K230、MSPM0 和驱动板共地。

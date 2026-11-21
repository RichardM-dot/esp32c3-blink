# esp32c3-blink — 裸机 FreeRTOS 单核（ESP32-C3）

不依赖 ESP-IDF 构建系统，直接用 `riscv32-esp-elf` 工具链 + 厂商内核源码编译一个
单核 RISC-V FreeRTOS 镜像。

## 当前状态

| 项目 | 状态 |
| --- | --- |
| 编译 / 链接 | 通过 |
| 静态自检 `make verify` | 全部通过（29 项） |
| LED demo | GPIO8，500 ms 翻转（无 UART 输出） |
| 上板实测 | **尚未进行** |

## 关键配置

| 项 | 值 |
| --- | --- |
| 架构 | RISC-V RV32IMC（`-march=rv32imc_zicsr_zifencei -mabi=ilp32 -mcmodel=medany`） |
| RTOS | FreeRTOS **V10.5.1**（IDF 内核源码 vendored，`configNUMBER_OF_CORES = 1`） |
| 自定义端口 | `Drivers/FreeRTOS/baremetal/port`（port.c / portasm.S / portmacro.h） |
| tick 源 | SYSTIMER unit0 target0 周期模式，**16 MHz 计数 / 1 kHz tick**（与 CPU 主频无关，精度稳） |
| trap | `mtvec` 直接模式 → `freertos_risc_v_trap_handler`，CPU 中断 6 进 trap 调 `xTaskIncrementTick()` |
| FreeRTOS 配置 | CPU 160 MHz、tick 1 kHz、heap_4 **64 KB** |
| LED | **GPIO8**（`GPIO->enable_w1ts / out_w1ts / out_w1tc` 寄存器直写） |
| 镜像 | magic 0xE9、chip_id 0x0005、DIO / 40 MHz / 4 MB、wp_pin 0xEE、尾部 SHA256 |

## 内存布局（重要）

ESP32-C3 的 400 KB SRAM 通过两条总线以不同地址暴露：

```
IRAM 0x4037C000..0x403E0000
DRAM 0x3FC7C000..0x3FCE0000
DRAM 地址 = IRAM 地址 - 0x700000
```

* 最低 16 KB（0x4037C000..0x40380000）是**指令 cache，不能放代码**——
  链接脚本断言 `.text` 必须从 0x40380000 开始；
* 所有可加载内容（text/rodata/data）在 IRAM，单 PT_LOAD 段；
* `.dram_skip` 把 DRAM 位置计数器推过整个 IRAM 镜像的别名区，`.bss`
  从别名区之后开始（否则清 bss 会擦掉代码）；
* 栈固定在 DRAM 顶部 16 KB（`_stack_bottom` 0x3FCCA710 .. `_stack_top`
  0x3FCCE710），链接期断言 `.bss` 不会长进栈里。

`make verify` 会把这几条不变量全部重新算一遍。

## 构建 / 自检

```sh
make            # 编译 + 打包 + 29 项静态自检
make verify     # 只跑自检
make size
make dis
make symbols
make clean
```

自检覆盖：ELF 端序/架构、关键符号存在性、`_start` 位置避开 I-cache、trap
handler 对齐、IRAM/DRAM 别名不重叠、`.bss` 不侵入栈、`pxCurrentTCBs` 为 1 项、
镜像头（magic/entry/chip_id/flash mode）、段加载位置、16 字节对齐、SHA256。

### 当前镜像关键符号（重建后地址可能微移，以 `nm` 实测为准）

| 符号 | 地址 |
| --- | --- |
| `_start` | 0x40380000（入口，同时也是 .text 起点） |
| `freertos_risc_v_trap_handler` | 0x40380040 |
| `vTaskStartScheduler` | 0x40381150 |
| `pxCurrentTCBs` | 0x3FC91C20 |
| `.bss` | 0x3FC81AC0 .. 0x3FC91C60 |

固件：`build/esp32c3-blink.esp.bin`（约 6912 B）。

## 烧写

```sh
esptool.py --chip esp32c3 -p /dev/tty.usbmodem* \
  write_flash 0x0 build/esp32c3-blink.esp.bin
```

必须烧到 **0x0**（ROM 直接启动）。镜像里没有 ESP-IDF 的 app descriptor，放到
0x10000 的 app 分区会被 IDF 二级 bootloader 拒绝。

进下载模式：按住 BOOT 再按 RESET。**验证方式：GPIO8 LED 以 500 ms 周期翻转**
（本工程没有接 UART 输出）。

## 已知限制

* 未上板实测，所有结论来自静态自检（`make verify` / `nm` / `objdump` / SHA256）。
* ISR 直接使用被打断任务的栈，没有独立中断栈。
* 未保存浮点上下文（当前 demo 为 RV32IMC，不含浮点指令）。
* 自包含性已验证：外部 `esp-idf` 目录改名后仍可完整编译（见《说明文档》6.4 节）。

## 头文件维护

工程 `Drivers/idf/` 内是按需复制的 IDF 头文件（soc/hal/esp_common/esp_rom/
esp_hal_uart/esp_hal_gpio）。IDF 升级后按《说明文档》8.1 节手动同步。

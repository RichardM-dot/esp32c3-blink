/* esp32c3.ld — ESP32-C3 IRAM/DRAM 布局
 *
 * C3 的 400 KB SRAM 通过两条总线以不同地址暴露：
 *
 *     IRAM 0x4037C000..0x403E0000
 *     DRAM 0x3FC7C000..0x3FCE0000
 *     DRAM 地址 = IRAM 地址 - 0x700000
 *
 * 其中最低 16 KB（0x4037C000..0x40380000）是 16 KB 指令 cache，不能放代码；
 * 可执行/可共享的窗口从 0x40380000（IRAM 侧）/ 0x3FC80000（DRAM 侧）开始。
 *
 * 两个窗口指向同一块物理 SRAM，所以 .bss 必须先用 .dram_skip 跳过整个
 * IRAM 镜像的别名区，否则清零 .bss 会把代码擦掉。
 *
 * 上限取 0x4E710：0x403CE710 起是二级 bootloader 的保留区（与 IDF 一致）。
 * 栈固定在 DRAM 顶部 16 KB，链接期断言 .bss 不会长进栈里。
 */
ENTRY(_start)

_diram_i_start = 0x40380000;

MEMORY {
  IRAM (rx) : ORIGIN = 0x40380000, LENGTH = 0x4E710
  DRAM (rw) : ORIGIN = 0x3FC80000, LENGTH = 0x4E710
}

SECTIONS {
  .text : {
    KEEP(*(.init.text))
    *(.text) *(.text*)
    *(.rodata) *(.rodata*)
    *(.srodata) *(.srodata*)
  } > IRAM

  /* 初始化数据随镜像放在 IRAM，避免第二个 PT_LOAD 段和别名冲突。 */
  .data : ALIGN(16) {
    *(.data) *(.data.*)
    *(.sdata) *(.sdata.*)
  } > IRAM

  _iram_end = .;

  .dram_skip (NOLOAD) : {
    . = ORIGIN(DRAM) + MAX(_iram_end, _diram_i_start) - _diram_i_start;
  } > DRAM

  .bss (NOLOAD) : ALIGN(16) {
    _bss_start = .;
    *(.bss) *(.bss.*)
    *(.sbss) *(.sbss.*)
    *(COMMON)
    . = ALIGN(16);
    _bss_end = .;
  } > DRAM

  _stack_top = ORIGIN(DRAM) + LENGTH(DRAM);
  _stack_bottom = _stack_top - 0x4000;

  ASSERT(ADDR(.text) == 0x40380000,
         "code must start after the 16 KiB instruction cache")
  ASSERT(_bss_end <= _stack_bottom,
         "DRAM data overruns the reserved stack area")
}

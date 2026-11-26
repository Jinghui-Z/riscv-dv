/* Copyright 2018 Google LLC
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http:*www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

OUTPUT_ARCH( "riscv" )
ENTRY(_start)

SECTIONS
{
  . = 0x80000000;
  .text : { *(.text) }
  /* Populated by either store restriction mode. Keep DV-generated random write
     targets outside text, including any explicitly named text subsections. */
  .text.extra : { *(.text.*) }
  .dv_store_data ALIGN(0x1000) : {
    __dv_store_data_start = .;
    *(.dv_store_data.*)
    __dv_store_data_end = .;
  }
  ASSERT(SIZEOF(.dv_store_data) == 0 ||
         ADDR(.dv_store_data) >= ADDR(.text) + SIZEOF(.text),
         "restricted store data overlaps text")
  . = ALIGN(0x1000);
  .tohost : { *(.tohost) }
  . = ALIGN(0x1000);
  .page_table : { *(.page_table) }
  .rodata : { *(.rodata .rodata.* .srodata .srodata.*) }
  .data : { *(.data .data.* .sdata .sdata.* .dv_hypertest_context) }
  .user_stack : { *(.user_stack) }
  .kernel_data : { *(.kernel_data) }
  .kernel_stack : { *(.kernel_stack) }
  .bss : {
    __dv_hypertest_bss_start = .;
    *(.bss .bss.* .sbss .sbss.*) *(COMMON)
    __dv_hypertest_bss_end = .;
  }
  .dv_hypertest_stack (NOLOAD) : { *(.dv_hypertest_stack) }
  .dv_hypertest_selftest_data : { *(.dv_hypertest_selftest_data) }
  .dv_hypertest_selftest_stack (NOLOAD) : { *(.dv_hypertest_selftest_stack) }
  _end = .;
  ASSERT(!DEFINED(dv_hypertest_call) || _end <= 0x88000000,
         "DV/hypertest image overlaps hypertest physical-page fixtures")
}

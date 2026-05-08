// SPDX-FileCopyrightText: 2026 Dragorn421
// SPDX-License-Identifier: CC0-1.0

#define F3DEX2_EXEC_VERBOSE 0

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include <libdragon.h>

#include "libultra/libultra_defs.h"

#include "f3dex2_exec.h"

extern uint64_t gspF3DZEX2_NoN_PosLight_fifoTextStart[];
extern uint64_t gspF3DZEX2_NoN_PosLight_fifoDataStart[];

#define ARRAY_COUNT(arr) (s32)(sizeof(arr) / sizeof(arr[0]))
#define ALIGNED(n) __attribute__((aligned(n)))

ALIGNED(16)
uint64_t gGfxSPTaskOutputBuffer[0x3000];

ALIGNED(16)
uint64_t gGfxSPTaskYieldBuffer[OS_YIELD_DATA_SIZE / sizeof(uint64_t)];

ALIGNED(16)
uint64_t gGfxSPTaskStack[SP_DRAM_STACK_SIZE64];

static OSTask sTmpTask;

OSTask *_VirtualToPhysicalTask(OSTask *intp) {
  OSTask *tp = &sTmpTask;

  memcpy(tp, intp, sizeof(OSTask));

#define _osVirtualToPhysical(ptr)                                              \
  if (ptr != NULL) {                                                           \
    /* compared to libultra this doesn't handle TLB but whatever */            \
    ptr = (void *)PhysicalAddr(ptr);                                           \
  }

  _osVirtualToPhysical(tp->t.ucode);
  _osVirtualToPhysical(tp->t.ucode_data);
  _osVirtualToPhysical(tp->t.dram_stack);
  _osVirtualToPhysical(tp->t.output_buff);
  _osVirtualToPhysical(tp->t.output_buff_size);
  _osVirtualToPhysical(tp->t.data_ptr);
  _osVirtualToPhysical(tp->t.yield_data_ptr);

  return tp;
}

void run_rsp_task(OSTask *intp) {
  OSTask *tp = _VirtualToPhysicalTask(intp);

  if (tp->t.flags & OS_TASK_YIELDED) {
    tp->t.ucode_data = tp->t.yield_data_ptr;
    tp->t.ucode_data_size = tp->t.yield_data_size;
    intp->t.flags &= ~OS_TASK_YIELDED;

    if (tp->t.flags & OS_TASK_LOADABLE) {
      assert(false);
      // tp->t.ucode = (u64 *)IO_READ((u32)intp->t.yield_data_ptr +
      // OS_YIELD_DATA_SIZE - 4);
    }
  }
  data_cache_hit_writeback(tp, sizeof(OSTask));
  *SP_STATUS = SP_WSTATUS_CLEAR_SIG0 | SP_WSTATUS_CLEAR_SIG1 |
               SP_WSTATUS_CLEAR_SIG2 | SP_WSTATUS_SET_INTR_BREAK;

  assert(*SP_STATUS & SP_STATUS_HALTED);
  *SP_PC = 0x80;
  assert(sizeof(OSTask) == 0x40);
  assert(tp->t.ucode_data_size <= 0xFC0);
  rsp_load_data(tp->t.ucode_data, tp->t.ucode_data_size, 0);
  rsp_load_data(tp, sizeof(OSTask), 0xFC0);
  // rsp_load_code(tp->t.ucode_boot, 0x80, 0); // useless
  rsp_load_code(tp->t.ucode, 0xF80, 0x80);
  *SP_STATUS = SP_WSTATUS_SET_INTR_BREAK | SP_WSTATUS_CLEAR_SSTEP |
               SP_WSTATUS_CLEAR_BROKE | SP_WSTATUS_CLEAR_HALT;
}

unsigned int n_seen_sp_intr = 0;
unsigned int n_seen_dp_intr = 0;

struct {
  void (*callback)(void *callback_arg);
  void *callback_arg;
} callback_queue[4];
unsigned int callback_queue_len = 0;
unsigned int callback_queue_rdpos = 0;
unsigned int callback_queue_wrpos = 0;

void check_seen_interrupts() {
  disable_interrupts();
#if F3DEX2_EXEC_VERBOSE
  fprintf(stderr, "check_seen_interrupts\n");
#endif
  bool seen_sp_and_dp_intr = n_seen_sp_intr != 0 && n_seen_dp_intr != 0;
  if (seen_sp_and_dp_intr) {
#if F3DEX2_EXEC_VERBOSE
    fprintf(stderr, "check_seen_interrupts: seen_sp_and_dp_intr yes\n");
#endif
    n_seen_sp_intr--;
    n_seen_dp_intr--;
    if (callback_queue_len != 0) {
#if F3DEX2_EXEC_VERBOSE
      fprintf(stderr, "check_seen_interrupts: callback()\n");
#endif
      callback_queue_len--;
      callback_queue[callback_queue_rdpos].callback(
          callback_queue[callback_queue_rdpos].callback_arg);
      callback_queue_rdpos++;
      callback_queue_rdpos %= ARRAY_COUNT(callback_queue);
    }
  }
  enable_interrupts();
}

void f3dex2_exec_SP_interrupt_handler(void) {
#if F3DEX2_EXEC_VERBOSE
  fprintf(stderr, "f3dex2_exec_SP_interrupt_handler\n");
#endif
  n_seen_sp_intr++;
  check_seen_interrupts();
}

void f3dex2_exec_DP_interrupt_handler(void) {
#if F3DEX2_EXEC_VERBOSE
  fprintf(stderr, "f3dex2_exec_DP_interrupt_handler\n");
#endif
  n_seen_dp_intr++;
  check_seen_interrupts();
}

static bool f3dex2_exec_initialized = false;

void f3dex2_exec_init() {
  if (f3dex2_exec_initialized)
    return;
  f3dex2_exec_initialized = true;

  set_SP_interrupt(1);
  set_DP_interrupt(1);

  register_SP_handler(f3dex2_exec_SP_interrupt_handler);
  register_DP_handler(f3dex2_exec_DP_interrupt_handler);
}

void f3dex2_exec_task(void *workBuffer, void *workBufferEnd,
                      void (*callback)(void *callback_arg),
                      void *callback_arg) {
  OSTask taskData;
  OSTask_t *task = &taskData.t;

  task->type = 1; // M_GFXTASK
  task->flags = OS_TASK_LOADABLE;
  // task->ucode_boot = rspbootTextStart;
  // task->ucode_boot_size = (uint32_t)rspbootTextEnd -
  // (uint32_t)rspbootTextStart;
  task->ucode = gspF3DZEX2_NoN_PosLight_fifoTextStart;
  task->ucode_data = gspF3DZEX2_NoN_PosLight_fifoDataStart;
  task->ucode_size = SP_UCODE_SIZE;
  task->ucode_data_size = SP_UCODE_DATA_SIZE;
  task->dram_stack = gGfxSPTaskStack;
  task->dram_stack_size = sizeof(gGfxSPTaskStack);
  task->output_buff = gGfxSPTaskOutputBuffer;
  task->output_buff_size =
      gGfxSPTaskOutputBuffer + ARRAY_COUNT(gGfxSPTaskOutputBuffer);
  task->data_ptr = (u64 *)workBuffer;

  task->data_size = (uintptr_t)workBufferEnd - (uintptr_t)workBuffer;

  task->yield_data_ptr = gGfxSPTaskYieldBuffer;

  task->yield_data_size = sizeof(gGfxSPTaskYieldBuffer);

  data_cache_writeback_invalidate_all();

  run_rsp_task(&taskData);

  assert(callback_queue_len < ARRAY_COUNT(callback_queue));
  callback_queue_len++;
  callback_queue[callback_queue_wrpos].callback = callback;
  callback_queue[callback_queue_wrpos].callback_arg = callback_arg;
  callback_queue_wrpos++;
  callback_queue_wrpos %= ARRAY_COUNT(callback_queue);
}

// SPDX-FileCopyrightText: 2026 Dragorn421
// SPDX-License-Identifier: CC0-1.0

#ifndef F3DEX2_EXEC_H
#define F3DEX2_EXEC_H

void f3dex2_exec_init();

void f3dex2_exec_task(void *workBuffer, void *workBufferEnd,
                      void (*callback)(void *callback_arg), void *callback_arg);

#endif

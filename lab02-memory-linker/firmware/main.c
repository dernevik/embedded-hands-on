/**
 ******************************************************************************
 * @file           : main.c
 * @author         : ChatGPT / Markus
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

#include <stdint.h>
#include <stdlib.h>

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

int initialized_global = 42;
int uninitialized_global;
const int constant_global = 123;

volatile int result;

__attribute__((noinline, optimize("O1")))
static int simple_local(void)
{
    int x = 42;
    return x;
}

int main(void)
{
    const volatile int *read_constant = &constant_global;

    int local_result = simple_local();

    static volatile unsigned char diagnostic_buffer[256];
    diagnostic_buffer[0] = 0x7E;


    unsigned char *heap_ptr = malloc(16);

    if (heap_ptr != NULL)
    {
        heap_ptr[0] = 0x5A;
    }

    unsigned char *heap_ptr2 = malloc(16);

    if (heap_ptr2 != NULL)
    {
        heap_ptr2[0] = 0xA5;
    }

    free(heap_ptr);
    heap_ptr = NULL;

    unsigned char *heap_ptr3 = malloc(16);

    if (heap_ptr3 != NULL)
    {
        heap_ptr3[0] = 0x3C;
    }

    result = initialized_global
           + uninitialized_global
           + *read_constant
		   + local_result;



    while (1)
    {
    }
}

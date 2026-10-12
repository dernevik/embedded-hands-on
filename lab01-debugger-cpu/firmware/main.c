/**
 ******************************************************************************
 * @file           : main.c
 * @author         : Modified by Markus D
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 *
 ******************************************************************************
 */

#include <stdint.h>

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
  #warning "FPU is not initialized, but the project is compiling for an FPU. Please initialize the FPU before use."
#endif

volatile int counter = 0;

static void bar(void)
{
    counter++;
}

static void foo(void)
{
    bar();

   /*
    if (counter == 3)
    {
        bar();
    }
    */
}

int main(void)
{
    while (1)
    {
        foo();
    }
}

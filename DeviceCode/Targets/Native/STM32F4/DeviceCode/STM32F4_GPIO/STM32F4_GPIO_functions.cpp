////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
//
//  Copyright (c) Microsoft Corporation. All rights reserved.
//  Implementation for STM32F4: Copyright (c) Oberon microsystems, Inc.
//
//  *** GPIO Driver ***
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <tinyhal.h>
#include <cores\arm\include\cpu.h>

#ifdef STM32F4XX
#include "..\stm32f4xx.h"
#else
#include "..\stm32f2xx.h"
#endif

//--//

#define STM32F4_Gpio_MaxPins (TOTAL_GPIO_PORT * 16)
#define STM32F4_Gpio_MaxInt 16

// indexed port configuration access
#define Port(port) ((GPIO_TypeDef *) (GPIOA_BASE + (port << 10)))

static UINT32 g_debounceTicks;
static UINT16 g_pinReserved[TOTAL_GPIO_PORT]; //  1 bit per pin

static GPIO_INTERRUPT_SERVICE_ROUTINE g_ISR[STM32F4_Gpio_MaxInt]; // interrupt handlers
static void* g_ISR_Param[STM32F4_Gpio_MaxInt]; // interrupt handler parameters


/*
 * Interrupt Handler
 */

void STM32F4_GPIO_ISR (int num)  // 0 <= num <= 15
{
    INTERRUPT_START
    
    GPIO_INTERRUPT_SERVICE_ROUTINE isr = g_ISR[num];
    if (isr) {
        int port = (SYSCFG->EXTICR[num >> 2] >> ((num & 0x3) << 2)) & 0xF;
        isr((port << 4) | num, (Port(port)->IDR >> num) & 1, g_ISR_Param[num]);
    }
    
    INTERRUPT_END
}

void STM32F4_GPIO_Interrupt0 (void* param) // EXTI0
{
    EXTI->PR = 1 << 0;   // reset pending bit
    STM32F4_GPIO_ISR(0);
}

void STM32F4_GPIO_Interrupt1 (void* param) // EXTI1
{
    EXTI->PR = 1 << 1;   // reset pending bit
    STM32F4_GPIO_ISR(1);
}

void STM32F4_GPIO_Interrupt2 (void* param) // EXTI2
{
    EXTI->PR = 1 << 2;   // reset pending bit
    STM32F4_GPIO_ISR(2);
}

void STM32F4_GPIO_Interrupt3 (void* param) // EXTI3
{
    EXTI->PR = 1 << 3;   // reset pending bit
    STM32F4_GPIO_ISR(3);
}

void STM32F4_GPIO_Interrupt4 (void* param) // EXTI4
{
    EXTI->PR = 1 << 4;   // reset pending bit
    STM32F4_GPIO_ISR(4);
}

void STM32F4_GPIO_Interrupt5 (void* param) // EXTI5 - EXTI9
{
    UINT32 pending = EXTI->PR & 0x03E0; // pending bits 5..9
    EXTI->PR = pending;    // reset pending bits 5..9
    pending &= EXTI->IMR;  // remove masked bits
    int num = 5; pending >>= 5;
    do {
        if (pending & 1) STM32F4_GPIO_ISR(num);
        num++; pending >>= 1;
    } while (pending);
}

void STM32F4_GPIO_Interrupt10 (void* param) // EXTI10 - EXTI15
{
    UINT32 pending = EXTI->PR & 0xFC00; // pending bits 10..15
    EXTI->PR = pending;    // reset pending bits 10..15
    pending &= EXTI->IMR;  // remove masked bits
    int num = 10; pending >>= 10;
    do {
        if (pending & 1) STM32F4_GPIO_ISR(num);
        num++; pending >>= 1;
    } while (pending);
}

BOOL STM32F4_GPIO_Set_Interrupt( UINT32 pin, GPIO_INTERRUPT_SERVICE_ROUTINE ISR, void* ISR_Param, GPIO_INT_EDGE mode)
{
    UINT32 num = pin & 0x0F;
    UINT32 bit = 1 << num;
    UINT32 shift = (num & 0x3) << 2; // 4 bit fields
    UINT32 idx = num >> 2;
    UINT32 mask = 0xF << shift;
    UINT32 config = (pin >> 4) << shift; // port number configuration
    
    GLOBAL_LOCK(irq);
    
    if (ISR) {
        if ((SYSCFG->EXTICR[idx] & mask) != config) {
            if (EXTI->IMR & bit) return FALSE; // interrupt in use
            SYSCFG->EXTICR[idx] = SYSCFG->EXTICR[idx] & ~mask | config;
        }
        g_ISR[num] = ISR;
        g_ISR_Param[num] = ISR_Param;
        if (mode == GPIO_INT_EDGE_HIGH || mode == GPIO_INT_EDGE_BOTH) {
            EXTI->RTSR |= bit;
        } else {
            EXTI->RTSR &= ~bit;
        }
        if (mode == GPIO_INT_EDGE_LOW || mode == GPIO_INT_EDGE_BOTH) {
            EXTI->FTSR |= bit;
        } else {
            EXTI->FTSR &= ~bit;
        }
        EXTI->PR = bit; // remove pending interrupt
        EXTI->IMR |= bit; // enable interrupt
    } else if ((SYSCFG->EXTICR[idx] & mask) == config) {
        EXTI->IMR &= ~bit; // disable interrupt
        g_ISR[num] = NULL;
    }
    return TRUE;
}


// mode:  0: input,  1: output,  2: alternate, 3: analog
// alternate: od | AF << 4 | speed << 8
void STM32F4_GPIO_Pin_Config( GPIO_PIN pin, UINT32 mode, GPIO_RESISTOR resistor, UINT32 alternate )
{
    GPIO_TypeDef* port = Port(pin >> 4); // pointer to the actual port registers
    pin &= 0x0F; // bit number
    UINT32 bit = 1 << pin;
    UINT32 shift = pin << 1; // 2 bits / pin
    UINT32 mask = 0x3 << shift;
    UINT32 pull = 0;
    if (resistor == RESISTOR_PULLUP) pull = GPIO_PUPDR_PUPDR0_0;
    if (resistor == RESISTOR_PULLDOWN) pull = GPIO_PUPDR_PUPDR0_1;
    pull <<= shift;
    mode <<= shift;
    UINT32 speed = (alternate >> 8) << shift;
    UINT32 altSh = (pin & 0x7) << 2; // 4 bits / pin
    UINT32 altMsk = 0xF << altSh;
    UINT32 idx = pin >> 3;
    UINT32 af = ((alternate >> 4) & 0xF) << altSh;
    
    GLOBAL_LOCK(irq);
    
    port->MODER = port->MODER & ~mask | mode;
    port->PUPDR = port->PUPDR & ~mask | pull;
    port->OSPEEDR = port->OSPEEDR & ~mask | speed;
    port->AFR[idx] = port->AFR[idx] & ~altMsk | af;
    if (alternate & 1) { // open drain
        port->OTYPER |= bit;
    } else {
        port->OTYPER &= ~bit;
    }
}

BOOL CPU_GPIO_Initialize()
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();

    CPU_GPIO_SetDebounce(20); // ???
    
    for (int i = 0; i < TOTAL_GPIO_PORT; i++) {
        g_pinReserved[i] = 0;
    }
    
    EXTI->IMR = 0; // disable all external interrups;
    CPU_INTC_ActivateInterrupt(EXTI0_IRQn, STM32F4_GPIO_Interrupt0, 0);
    CPU_INTC_ActivateInterrupt(EXTI1_IRQn, STM32F4_GPIO_Interrupt1, 0);
    CPU_INTC_ActivateInterrupt(EXTI2_IRQn, STM32F4_GPIO_Interrupt2, 0);
    CPU_INTC_ActivateInterrupt(EXTI3_IRQn, STM32F4_GPIO_Interrupt3, 0);
    CPU_INTC_ActivateInterrupt(EXTI4_IRQn, STM32F4_GPIO_Interrupt4, 0);
    CPU_INTC_ActivateInterrupt(EXTI9_5_IRQn, STM32F4_GPIO_Interrupt5, 0);
    CPU_INTC_ActivateInterrupt(EXTI15_10_IRQn, STM32F4_GPIO_Interrupt10, 0);

    return TRUE;
}

BOOL CPU_GPIO_Uninitialize()
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();

    EXTI->IMR = 0; // disable all external interrups;
    CPU_INTC_DeactivateInterrupt(EXTI0_IRQn);
    CPU_INTC_DeactivateInterrupt(EXTI1_IRQn);
    CPU_INTC_DeactivateInterrupt(EXTI2_IRQn);
    CPU_INTC_DeactivateInterrupt(EXTI3_IRQn);
    CPU_INTC_DeactivateInterrupt(EXTI4_IRQn);
    CPU_INTC_DeactivateInterrupt(EXTI9_5_IRQn);
    CPU_INTC_DeactivateInterrupt(EXTI15_10_IRQn);
    
    return TRUE;
}

UINT32 CPU_GPIO_Attributes( GPIO_PIN pin )
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    if(pin < STM32F4_Gpio_MaxPins) {
        return GPIO_ATTRIBUTE_INPUT | GPIO_ATTRIBUTE_OUTPUT;
    }
    return GPIO_ATTRIBUTE_NONE;
}

/* 
 * alternate:
 * GPIO_ALT_PRIMARY: GPIO
 * GPIO_ALT_MODE_1: Analog
 * GPIO_ALT_MODE_2 | AF << 4 | speed << 8: Alternate Function
 * GPIO_ALT_MODE_3 | AF << 4 | speed << 8: Alternate Function with open drain
 * speed: 0: 2MHZ, 1: 25MHz, 2: 50MHz, 3: 100MHz
 */
void CPU_GPIO_DisablePin( GPIO_PIN pin, GPIO_RESISTOR resistor, UINT32 output, GPIO_ALT_MODE alternate )
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    if (pin < STM32F4_Gpio_MaxPins) {
        UINT32 mode = output;
        UINT32 altMode = (UINT32)alternate & 0x0F;
        if (altMode == 1) mode = 3; // analog
        else if (altMode) mode = 2; // alternate pin function
        STM32F4_GPIO_Pin_Config(pin, mode, resistor, (UINT32)alternate);
        STM32F4_GPIO_Set_Interrupt(pin, NULL, 0, GPIO_INT_NONE); // disable interrupt
    }
}

void CPU_GPIO_EnableOutputPin( GPIO_PIN pin, BOOL initialState )
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    if (pin < STM32F4_Gpio_MaxPins) {
        CPU_GPIO_SetPinState(pin, initialState);
        STM32F4_GPIO_Pin_Config(pin, 1, RESISTOR_DISABLED, 0); // general purpose output
        STM32F4_GPIO_Set_Interrupt(pin, NULL, 0, GPIO_INT_NONE); // disable interrupt
    }
}

BOOL CPU_GPIO_EnableInputPin( GPIO_PIN pin, BOOL GlitchFilterEnable, GPIO_INTERRUPT_SERVICE_ROUTINE ISR, GPIO_INT_EDGE edge, GPIO_RESISTOR resistor )
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    return CPU_GPIO_EnableInputPin2( pin, GlitchFilterEnable, ISR, 0, edge, resistor );
}

BOOL CPU_GPIO_EnableInputPin2( GPIO_PIN pin, BOOL GlitchFilterEnable, GPIO_INTERRUPT_SERVICE_ROUTINE ISR, void* ISR_Param, GPIO_INT_EDGE edge, GPIO_RESISTOR resistor )
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    if (pin >= STM32F4_Gpio_MaxPins) return FALSE;
    STM32F4_GPIO_Pin_Config(pin, 0, resistor, 0); // input
    
    if (edge > GPIO_INT_EDGE_BOTH) return FALSE;
    return STM32F4_GPIO_Set_Interrupt(pin, ISR, ISR_Param, edge);
}

BOOL CPU_GPIO_GetPinState( GPIO_PIN pin )
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    if (pin >= STM32F4_Gpio_MaxPins) return FALSE;
    
    GPIO_TypeDef* port = Port(pin >> 4); // pointer to the actual port registers 
    return (port->IDR >> (pin & 0xF)) & 1;
}

void CPU_GPIO_SetPinState( GPIO_PIN pin, BOOL pinState )
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    if (pin < STM32F4_Gpio_MaxPins) {
        GPIO_TypeDef* port = Port(pin >> 4); // pointer to the actual port registers 
        UINT16 bit = 1 << (pin & 0x0F);
        if (pinState) port->BSRRL = bit; // set bit
        else port->BSRRH = bit; // reset bit
    }
}

//--//

BOOL CPU_GPIO_PinIsBusy( GPIO_PIN pin )  // busy == reserved
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    if (pin >= STM32F4_Gpio_MaxPins) return FALSE;
    int port = pin >> 4, sh = pin & 0x0F;
    return (g_pinReserved[port] >> sh) & 1;
}

BOOL CPU_GPIO_ReservePin( GPIO_PIN pin, BOOL fReserve )
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    if (pin >= STM32F4_Gpio_MaxPins) return FALSE;
    int port = pin >> 4, bit = 1 << (pin & 0x0F);
    GLOBAL_LOCK(irq);
    if (fReserve) {
        if (g_pinReserved[port] & bit) return FALSE; // already reserved
        g_pinReserved[port] |= bit;
    } else {
        g_pinReserved[port] &= ~bit;
    }
    return TRUE;
}

UINT32 CPU_GPIO_GetDebounce()
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    return CPU_TicksToTime(g_debounceTicks) / 10000; // ticks -> 100ns -> ms
}

BOOL CPU_GPIO_SetDebounce( INT64 debounceTimeMilliseconds )
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    if (debounceTimeMilliseconds > 0 && debounceTimeMilliseconds < 10000) {
        g_debounceTicks = CPU_MillisecondsToTicks((UINT32)debounceTimeMilliseconds);
        return TRUE;
    }
    return FALSE;
}

INT32 CPU_GPIO_GetPinCount()
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    return STM32F4_Gpio_MaxPins;
}

void CPU_GPIO_GetPinsMap( UINT8* pins, size_t size )
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    for (int i = 0; i < size && i < STM32F4_Gpio_MaxPins; i++) {
         pins[i] = GPIO_ATTRIBUTE_INPUT | GPIO_ATTRIBUTE_OUTPUT;
    }
}

UINT8 CPU_GPIO_GetSupportedResistorModes( GPIO_PIN pin )
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    return (1 << RESISTOR_DISABLED) | (1 << RESISTOR_PULLUP) | (1 << RESISTOR_PULLDOWN);
}

UINT8 CPU_GPIO_GetSupportedInterruptModes( GPIO_PIN pin )
{
    NATIVE_PROFILE_HAL_PROCESSOR_GPIO();
    return (1 << GPIO_INT_EDGE_LOW) | (1 << GPIO_INT_EDGE_HIGH ) | (1 << GPIO_INT_EDGE_BOTH);
}

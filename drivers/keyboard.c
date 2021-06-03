#include "keyboard.h"
#include "../cpu/ports.h"
#include "../cpu/isr.h"
#include "../libc/function.h"
#include <stdint.h>

#define SC_MAX 57

static key_press last_press;
static uint8_t press_flag;

static uint8_t alt_status = 0;
static uint8_t control_status = 0;
static uint8_t shift_status = 0;

static void keyboard_callback(registers_t *regs) {
    /* The PIC leaves us the scancode in port 0x60 */
    uint8_t scancode = port_byte_in(0x60);
    uint8_t release = 0;
    if (scancode & 0x80)
    {
        release = 1;
        scancode &= 0x7F;
    }
    if (scancode > SC_MAX) return;
    last_press.press_mode = release;
    last_press.key_data = scancode;
    if (scancode == KEY_LEFT_ALT)
    {
        if (release)
        {
            alt_status = 0;
        }
        else
        {
            alt_status = 1;
        }
    }
    else if (scancode == KEY_LEFT_CONTROL)
    {
        if (release)
        {
            control_status = 0;
        }
        else
        {
            control_status = 1;
        }
    }
    else if (scancode == KEY_RIGHT_SHIFT || scancode == KEY_LEFT_SHIFT)
    {
        if (release)
        {
            shift_status = 0;
        }
        else
        {
            shift_status = 1;
        }
    }
    last_press.alt = alt_status;
    last_press.control = control_status;
    last_press.shift = shift_status;
    press_flag = 1;
    
}

key_press readkey()
{
    while (press_flag == 0)
    {
         
    }
    press_flag = 0;
    return last_press;
}

void init_keyboard() {
   register_interrupt_handler(IRQ1, keyboard_callback); 
}

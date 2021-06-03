#include <stdint.h>

#ifndef KEYBOARD_H

#define KEYBOARD_H

#define KEY_ERROR 0
#define KEY_ESCAPE 1
#define KEY_D1 2
#define KEY_D2 3
#define KEY_D3 4
#define KEY_D4 5
#define KEY_D5 6
#define KEY_D6 7
#define KEY_D7 8
#define KEY_D8 9
#define KEY_D9 10
#define KEY_D0 11
#define KEY_SUBTRACT 12
#define KEY_EQUALS 13
#define KEY_BACKSPACE 14
#define KEY_TAB 15
#define KEY_Q 16
#define KEY_W 17
#define KEY_E 18
#define KEY_R 19
#define KEY_T 20
#define KEY_Y 21
#define KEY_U 22
#define KEY_I 23
#define KEY_O 24
#define KEY_P 25
#define KEY_OPEN_BRACKET 26
#define KEY_CLOSE_BRACKET 27
#define KEY_ENTER 28
#define KEY_LEFT_CONTROL 29
#define KEY_A 30
#define KEY_S 31
#define KEY_D 32
#define KEY_F 33
#define KEY_G 34
#define KEY_H 35
#define KEY_J 36
#define KEY_K 37
#define KEY_L 38
#define KEY_SEMICOLON 39
#define KEY_QUOTE 40
#define KEY_APOSTROPHE 41
#define KEY_LEFT_SHIFT 42
#define KEY_BACKSLASH 43
#define KEY_Z 44
#define KEY_X 45
#define KEY_C 46
#define KEY_V 47
#define KEY_B 48
#define KEY_N 49
#define KEY_M 50
#define KEY_COMMA 51
#define KEY_PERIOD 52
#define KEY_SLASH 53
#define KEY_RIGHT_SHIFT 54
#define KEY_ASTERISK 55
#define KEY_LEFT_ALT 56
#define KEY_SPACEBAR 57

#define PRESS_MODE_KEY_UP 1
#define PRESS_MODE_KEY_DOWN 0

typedef struct key_press
{
	uint8_t control;
	uint8_t shift;
	uint8_t alt;
	uint8_t press_mode;
	uint8_t key_data;
}key_press;

void init_keyboard();
key_press readkey();

#endif // !KEYBOARD_H
#include "io.h"
#include "../drivers/screen.h"
#include "../drivers/keyboard.h"
#include "../libc/string.h"
#include "../libc/mem.h"

const char sc_ascii_lower[] = { '\0', '\0', '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', '-', '=', '\0', '\0', 'q', 'w', 'e', 'r', 't', 'y',
		'u', 'i', 'o', 'p', '[', ']', '\0', '\0', 'a', 's', 'd', 'f', 'g',
		'h', 'j', 'k', 'l', ';', '\'', '`', '\0', '\\', 'z', 'x', 'c', 'v',
		'b', 'n', 'm', ',', '.', '/', '\0', '\0', '\0', ' ' };

const char sc_ascii_upper[] = { '?', '\0', '!', '@', '#', '$', '%', '^',
	'&', '*', '(', ')', '_', '+', '\0', '\0', 'Q', 'W', 'E', 'R', 'T', 'Y',
		'U', 'I', 'O', 'P', '{', '}', '\0', '\0', 'A', 'S', 'D', 'F', 'G',
		'H', 'J', 'K', 'L', ';', '\"', '~', '\0', '|', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', '<', '>', '?', '\0', '\0', '\0', ' ' };

void write(char str[])
{
	kprint(str);
}

void writeLine(char str[])
{
	kprint(str);
	kprint("\n");
}

void writeInt(int num)
{
	char numstr[25];
	int_to_ascii(num, numstr);
	kprint(numstr);
}

char* readAlloc()
{
	char* buffer = (char*)malloc(256);
	readLine(buffer);
	return buffer;
}

void readLine(char buffer[])
{
	buffer[0] = '\0';
	key_press keypress;
	while (1)
	{
		keypress = readkey();
		if (keypress.press_mode == PRESS_MODE_KEY_DOWN)
		{
			if (keypress.key_data == KEY_ENTER)
			{
				kprint("\n");
				return;
			}
			else if (keypress.key_data == KEY_BACKSPACE && strlen(buffer) > 0)
			{
				remove(buffer, strlen(buffer) - 1);
				kprint_backspace();
			}
			else if (!keypress.alt && !keypress.control)
			{
				char append_char;
				if (keypress.shift)
				{
					append_char = sc_ascii_upper[keypress.key_data];
				}
				else
				{
					append_char = sc_ascii_lower[keypress.key_data];
				}
				if (append_char != '\0')
				{
					append(buffer, append_char);
					char print_str[2] = { append_char, '\0' };
					kprint(print_str);
				}
			}
		}
	}
}
#include "string.h"
#include "mem.h"
#include "../kernel/io.h"
#include <stdint.h>

const char digits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E'};

void int_to_ascii(int n, char str[]) {
    int copy_n = n;
    str[0] = '\0';
    if (copy_n == 0)
    {
        append(str, '0');
    }
    else {
        while (copy_n > 0)
        {
            append(str, digits[copy_n % 10]);
            copy_n = copy_n - (copy_n % 10);
            copy_n = copy_n / 10;
        }
    }
    reverse(str);
}

void hex_to_ascii(int n, char str[]) {
    int copy_n = n;
    str[0] = '\0';
    while (copy_n > 0)
    {
        append(str, digits[copy_n % 16]);
        copy_n = copy_n - (copy_n % 16);
        copy_n = copy_n / 16;
    }
    append(str, 'x');
    append(str, '0');
    reverse(str);
}

/* K&R */
void reverse(char s[]) {
    int c, i, j;
    for (i = 0, j = strlen(s)-1; i < j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/* K&R */
int strlen(char s[]) {
    int i = 0;
    while (s[i] != '\0') ++i;
    return i;
}

void append(char s[], char n) {
    int len = strlen(s);
    s[len] = n;
    s[len+1] = '\0';
}

void insert(char s[], int index, char c)
{
    int len = strlen(s);
    s[len + 1] = '\0';

    for (int i = len - 1; i >= index; i--)
    {
        s[i + 1] = s[i];
    }
    s[index] = c;
}

void remove(char s[], int index)
{
    int len = strlen(s);
    for (int i = index + 1; i < len; i++)
    {
        s[i - 1] = s[i];
    }
    s[len - 1] = '\0';
}

/* K&R 
 * Returns <0 if s1<s2, 0 if s1==s2, >0 if s1>s2 */
int strcmp(char s1[], char s2[]) {
    int i;
    for (i = 0; s1[i] == s2[i]; i++) {
        if (s1[i] == '\0') return 0;
    }
    return s1[i] - s2[i];
}

void strcopy(char from[], char dest[], int offset, int length)
{
    for (int i = 0; i < length; i++)
    {
        dest[i + offset] = from[i + offset];
    }
    dest[offset + length] = '\0';
}

static char* input_buffer = NULL;

char* strtok(char* str, char delim)
{
    if (str != NULL)
    {
        input_buffer = str;
    }
    if (input_buffer == NULL || strlen(input_buffer) == 0)
    {
        return NULL;
    }

    char* tok = (char*)malloc(25);
    int i;
    for (i = 0; i < strlen(input_buffer); i++)
    {
        if (input_buffer[i] == delim)
            break;
        else
            tok[i] = input_buffer[i];
    }
    tok[i] = '\0';
    if (i == strlen(input_buffer)){
        input_buffer = NULL;
    }
    else {
        int j;
        for (j = i + 1; j < strlen(input_buffer); j++)
        {
            input_buffer[j - i-1] = input_buffer[j];
        }
        input_buffer[j - i - 1] = '\0';
    }
    return tok;
}
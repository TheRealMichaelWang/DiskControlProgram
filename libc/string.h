#ifndef STRINGS_H
#define STRINGS_H

void int_to_ascii(int n, char str[]);
void hex_to_ascii(int n, char str[]);
void reverse(char s[]);
int strlen(char s[]);
void append(char s[], char n);
void insert(char s[], int index, char c);
void remove(char s[], int index);
int strcmp(char s1[], char s2[]);
void strcopy(char from[], char dest[], int start, int length);
char* strtok(char* str, char delim);

#endif
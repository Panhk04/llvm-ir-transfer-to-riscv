#include <stdio.h>

// CACT Runtime Library

int get_int() {
    int x;
    scanf("%d", &x);
    return x;
}

char get_char() {
    char c;
    scanf("%c", &c);
    return c;
}

float get_float() {
    float x;
    scanf("%f", &x);
    return x;
}

void print_int(int x) {
    printf("%d", x);
}

void print_char(char c) {
    printf("%c", c);
}

void print_float(float x) {
    printf("%f", x);
} 
#include <stdint.h>

int print_screen(char *kbuf);
char *generate_kbuf();
int emulate_glcd_printf(char *msg, int x, int y);

int print_byte(char byte);
#define u8 uint8_t
static void set_pixel(u8 x, u8 y, u8 set);
static void zip_pixels();
static void alloc_char(char c, unsigned int x, unsigned int y);

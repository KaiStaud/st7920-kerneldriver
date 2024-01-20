#include "emulator.hpp"
#include "font_5x8.h"
#include "stdint.h"
#include "stdio.h"

/* From driver */
#define MAX_ROW 8
#define CHAR_WIDTH 6
#define START_PIXEL 0 // 0 for 8x8 2 for 5x8
#define CHECK_BIT(var, pos) ((var) & (1 << (pos)))

u8 fb[64][128];
u8 graphic_buffer[1042];

int print_screen(char *kbuf)
{
    // print the columns
    for (int i = 0; i < 128; i++)
    {
        if (i % 8 == 0)
        {
            printf("%3i", i);
        }
        else
        {
            printf("   ");
        }
    }
    printf("\r\n");
    for (int i = 0; i < 128; i++)
        printf("---");
    printf("\r\n");
    for (int i = 0; i < 1024; i++)
    {
        // Print row-numbers:
        if (i % (16 * 8) == 0)
        {
            printf("\r\n%2i|", i / 16);
        }
        else if (i % 16 == 0)
        {
            printf("\r\n  |");
        }
        else
        {
        }
        print_byte(kbuf[i]);
    }
}

char *generate_kbuf()
{
}

int emulate_glcd_printf(char *msg, int x, int y)
{
    uint8_t charcnt = 0;
    uint8_t row_offset = 0;
    uint8_t cnt = -1;
    while (msg[charcnt] != '\0')
    {
        cnt++;

        if (msg[charcnt] == '\n')
        {
            row_offset++;
            cnt--; // Don't advance 8 horizontal pixels in the next row!
        }
        else if (msg[charcnt] == '\r')
        {
            cnt = -1;
        }
        else
        {
            alloc_char(msg[charcnt], cnt + x, y + row_offset);
        }
        charcnt++;
    }
    zip_pixels();
    print_screen((char *)graphic_buffer);
}

int print_byte(char byte)
{
    int i;
    for (i = 7; i >= 0; i--)
    {
        if (byte & (1 << i))
        {
            printf(".");
        }
        else
        {
            printf(" ");
        }
    }
}

static void set_pixel(u8 x, u8 y, u8 set)
{
    fb[y][x] = set;
}

// Put each 8 pixels into one buffer-byte
static void zip_pixels()
{
    u8 lowbyte = 0, highbyte = 0;

    for (int y = 0; y < 64; y++)
    {
        // Durchlauf 0: Bits 0-7
        // Durchlauf 1: Bits 0-8
        for (int it = 0; it < 8; it++) // 16 byte pro zeile
        {
            // Bits [0..7]
            for (int bit_position = 0; bit_position < 8; bit_position++)
            {
                lowbyte |= (fb[y][it * 16 + bit_position] << (7 - bit_position));
            }
            // Bits [8..15]
            for (int bit_position = 8; bit_position < 16; bit_position++)
            {
                highbyte |= (fb[y][it * 16 + 8 + (bit_position - 8)] << (15 - bit_position));
            }

            graphic_buffer[y * 16 + it * 2] = lowbyte;
            graphic_buffer[y * 16 + it * 2 + 1] = highbyte;
            lowbyte = 0x00;
            highbyte = 0x00;
        }
    }
}

static void alloc_char(char c, unsigned int x, unsigned int y)
{
    u8 glyph;
    for (int row = 0; row < MAX_ROW; row++)
    {
        glyph = font[c][row];
        for (int pixel = START_PIXEL; pixel < 8; pixel++)
        {
            set_pixel(x * CHAR_WIDTH + pixel, y * 8 + row,
                      CHECK_BIT(glyph, pixel) ? (1) : (0)); // if pixel is set, set pixel in fb
        }
    }
}

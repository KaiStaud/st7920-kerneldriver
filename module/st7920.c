#include "st7920.h"
#include "font_5x8.h"
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/module.h>

#define rs_pin 16
#define MAX_ROW 8
#define CHAR_WIDTH 6
#define START_PIXEL 0 // 0 for 8x8 2 for 5x8
#define CHECK_BIT(var, pos) ((var) & (1 << (pos)))

/* Meta Information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Kai Staud");
MODULE_DESCRIPTION("char device driver for st7920 glcd");

/* file operation structure */
static struct file_operations glcd_fops = {
    .owner = THIS_MODULE,
    .open = glcd_open,
    .release = glcd_close,
    .read = glcd_read,
    .write = glcd_write,
    .unlocked_ioctl = glcd_ioctl,
};

struct st7920
{
    struct spi_device *client;
};

static struct of_device_id graphic_display_ids[] = {{
                                                        .compatible = "glcd,st7920",
                                                    },
                                                    {/* sentinel */}};
MODULE_DEVICE_TABLE(of, graphic_display_ids);

static struct spi_device_id st7920[] = {
    {"st7920", 0},
    {},
};
MODULE_DEVICE_TABLE(spi, st7920);

static struct spi_driver graphic_display = {
    .probe = glcd_probe,
    .remove = glcd_remove,
    .id_table = st7920,
    .driver =
        {
            .name = "st7920",
            .of_match_table = graphic_display_ids,
        },
};

/* private members*/
struct st7920 *dev;
struct spi_device *device;
u8 fb[64][128];
u8 graphic_buffer[1042];

/**
 * @brief This function is called on loading the driver
 */
static int glcd_probe(struct spi_device *client)
{

    u8 err;
    struct device *dev_ret;
    printk(KERN_INFO "st7920: setting up chardevice");
    // dynamically allocate device major number
    if (alloc_chrdev_region(&dev_number, MINOR_NUM_START, MINOR_NUM_COUNT, DEVICE_NAME) < 0)
    {
        printk(KERN_DEBUG "ERR: Failed to allocate major number \n");
        return -1;
    }

    // create a class structure
    glcd_class = class_create(THIS_MODULE, CLASS_NAME);

    if (IS_ERR(glcd_class))
    {
        unregister_chrdev_region(dev_number, MINOR_NUM_COUNT);
        printk(KERN_DEBUG "ERR: Failed to create class structure \n");

        return PTR_ERR(glcd_class);
    }

    // create a device and registers it with sysfs
    dev_ret = device_create(glcd_class, NULL, dev_number, NULL, DEVICE_NAME);

    if (IS_ERR(dev_ret))
    {
        class_destroy(glcd_class);
        unregister_chrdev_region(dev_number, MINOR_NUM_COUNT);
        printk(KERN_DEBUG "ERR: Failed to create device structure \n");

        return PTR_ERR(dev_ret);
    }

    // initialize a cdev structure
    cdev_init(&glcd_cdev, &glcd_fops);

    // add a character device to the system
    if (cdev_add(&glcd_cdev, dev_number, MINOR_NUM_COUNT) < 0)
    {
        device_destroy(glcd_class, dev_number);
        class_destroy(glcd_class);
        unregister_chrdev_region(dev_number, MINOR_NUM_COUNT);
        printk(KERN_DEBUG "ERR: Failed to add cdev \n");

        return -1;
    }

    device = client;
    /* Allocate driver data */
    printk(KERN_INFO "st7920: Loading lkm!\n");

    err = gpio_request(16, "rpi-gpio-16");
    if (err)
    {
        printk(KERN_ERR "st7920: gpio_request %d\n", err);
        return -1;
    }

    err = gpio_direction_output(16, 0);
    if (err)
    {
        printk(KERN_ERR "st7920: gpio_dir_output %d\n", err);
        //    gpio_free( 4 );
        return -1;
    }
    printk(KERN_INFO "st7920: gpio %d configured\n", rs_pin);
    init_lcd();
    set_graphic_mode();
    clear_screen();
    screen_test();
    printk(KERN_INFO "st7920: gpio-free %d", rs_pin);
    gpio_free(rs_pin);
    return 0;
}

/**
 * @brief This function is called on unloading the driver
 */

static void glcd_remove(struct spi_device *client)
{
    printk(KERN_INFO "st7920 - removing glcd from system!\n");

    cdev_del(&glcd_cdev);
    device_destroy(glcd_class, dev_number);
    class_destroy(glcd_class);
    // deallocate device major number
    unregister_chrdev_region(MAJOR(dev_number), MINOR_NUM_COUNT);

    gpio_free(rs_pin);
}

/* Low level spi functions*/
static void send_cmd(char cmd)
{
    // printk("st7920: sending cmd into the void\n");
    u8 buffer[3];
    buffer[0] = 0xf8 + (0 << 1);
    buffer[1] = cmd & 0xf0;
    buffer[2] = (cmd << 4) & 0xf0;
    gpio_set_value(rs_pin, 1);
    if (spi_write(device, buffer, 3) < 0)
    {
        printk(KERN_ERR "st7920: Error: Failed to transmit cmd!\n");
    }
    gpio_set_value(rs_pin, 0);
}

static int send_data(char data)
{
    u8 buffer[3];
    buffer[0] = 0xf8 + (1 << 1);
    buffer[1] = data & 0xf0;
    buffer[2] = (data << 4) & 0xf0;
    gpio_set_value(rs_pin, 1);
    if (spi_write(device, buffer, 3) < 0)
    {
        printk(KERN_ERR "st7920: Error: Failed to transmit data!\n");
        return -1;
    }
    gpio_set_value(rs_pin, 0);
    return 0;
}

static void init_lcd(void)
{
    printk("st7920: Initializing lcd\n");
    gpio_set_value(rs_pin, 0); //   self.write_pin(false);  // RESET=0
    msleep(10);                // wait for 10ms
    gpio_set_value(rs_pin, 1); // RESET=1
    msleep(50);                // wait for >40 ms
    send_cmd(0x30);            // 8bit mode
    usleep_range(110, 120);    //  >100us delay
    send_cmd(0x30);            // 8bit mode
    usleep_range(40, 50);      // >37us delay
    send_cmd(0x08);            // D=0, C=0, B=0
    fsleep(110);               // >100us delay
    send_cmd(0x01);            // clear screen
    msleep(12);                // >10 ms delay
    send_cmd(0x06);            // cursor increment right no shift
    msleep(1);                 // 1ms delay
    send_cmd(0x0C);            // D=1, C=0, B=0
    msleep(1);                 // 1ms delay
    send_cmd(0x02);            // return to home
    msleep(1);                 // 1ms delay
    printk(KERN_INFO "st7920: glcd initialized\n");
    // screen_test();
}

static void clear_screen(void)
{
    u8 nulled_fb[64][128];
    u8 nulled_graphic_buffer[1042];

    memcpy(fb, nulled_fb, sizeof(fb));
    memcpy(graphic_buffer, nulled_graphic_buffer, sizeof(graphic_buffer));

    /*
    for (int i = 0; i < 1042; i++)
    {
        graphic_buffer[i] = 0x00;
    }
    zip_pixels();
    */
    draw_fb();
}
static void set_graphic_mode(void)
{
    printk(KERN_INFO "st7920: switching to graphic mode");
    send_cmd(0x30); // 8 bit mode
    msleep(1);
    send_cmd(0x34); // switch to Extended instructions
    msleep(1);
    send_cmd(0x36); // enable graphics
    msleep(1);
}

static void block_write(void)
{

    for (int y = 0; y < 32; y++)
    {
        send_cmd(0x80 | y); // Vertical coordinate of the screen is specified first. (0-31)
        send_cmd(0x80 | 0); // Then horizontal coordinate of the screen is specified. (0-8)
        for (int x = 0; x < 16; x++)
        {
            send_data(graphic_buffer[x + y * 16]); // y
        }
        for (int x = 0; x < 16; x++)
        {
            send_data(graphic_buffer[x + y * 16 + 512]); // y
        }
    }
}

static void screen_test(void)
{
    printk("st7920: testing screen");
    // glcd_printf("HELLO WORLD",0,0);
    // glcd_printf("oh boy\r\nits really\r\nhot in here",1,2);
    zip_pixels();
    draw_fb();
}

static void set_pixel(u8 x, u8 y, u8 set)
{
    fb[y][x] = set;
}

static void draw_fb(void)
{
    block_write();
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

static void alloc_char(char c, unsigned int x, unsigned int y,unsigned int invert)
{
    u8 glyph;
    for (int row = 0; row < MAX_ROW; row++)
    {
        glyph = font[c][row];
        for (int pixel = START_PIXEL; pixel < 8; pixel++)
        {
            if(invert)
            {
            set_pixel(x * CHAR_WIDTH + pixel, y * 8 + row,
                      CHECK_BIT(glyph, pixel) ? (0) : (1)); // if pixel is set, set pixel in fb
            }
            else
            {
            set_pixel(x * CHAR_WIDTH + pixel, y * 8 + row,
                      CHECK_BIT(glyph, pixel) ? (1) : (0)); // if pixel is set, set pixel in fb
            }
        }
    }
}

static void glcd_printf(char *msg, unsigned int lineNumber, unsigned int nthCharacter,unsigned int invert)
{
    printk("Printing %s @ x=%i y=%i", msg, nthCharacter, lineNumber);
    u8 charcnt = 0;
    u8 row_offset = 0;
    u8 cnt = -1;
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
            alloc_char(msg[charcnt], cnt + nthCharacter, lineNumber + row_offset,invert);
        }
        charcnt++;
    }
    zip_pixels();
    draw_fb();
}

void print_bitmap(char *bmp, int size)
{
    memcpy(graphic_buffer, bmp, 1042);
    draw_fb();
}

void set_backlight(u8 on)
{
    u8 buffer[3];
    buffer[0] = 0x34;
    buffer[1] = 0x01;
    // buffer[2] = (cmd << 4) & 0xf0;
    buffer[2] = 0x08;
    // 0x034 0x001
    gpio_set_value(rs_pin, 1);
    if (spi_write(device, buffer, 3) < 0)
    {
        printk(KERN_ERR "st7920: Error: Failed to transmit cmd!\n");
    }
    gpio_set_value(rs_pin, 0);
}
/***  File Operations ****/
static int glcd_open(struct inode *p_inode, struct file *p_file)
{
    printk(KERN_INFO "glcd Driver: open()\n");
    return 0;
}
static int glcd_close(struct inode *p_inode, struct file *p_file)
{
    printk(KERN_INFO "glcd Driver: close()\n\n");
    return 0;
}
static ssize_t glcd_read(struct file *p_file, char __user *buf, size_t len, loff_t *off)
{
    printk(KERN_INFO "glcd Driver: read()\n");
    return 0;
}
static ssize_t glcd_write(struct file *p_file, const char __user *buf, size_t len, loff_t *off)
{

    char kbuf[MAX_BUF_LENGTH];
    unsigned long copyLength;

    if (buf == NULL)
    {
        printk(KERN_DEBUG "ERR: Empty user space buffer \n");
        return -ENOMEM;
    }

    memset(kbuf, '\0', sizeof(char) * MAX_BUF_LENGTH);
    copyLength = MIN((MAX_BUF_LENGTH - 1), (unsigned long)(len - 1));

    if (copyLength < 0)
        copyLength = 0;

    // Copy user space buffer to kernel space buffer
    if (copy_from_user(&kbuf, buf, copyLength))
    {
        printk(KERN_DEBUG "ERR: Failed to copy from user space buffer \n");
        return -EFAULT;
    }

    printk(KERN_INFO "***** value copied from user space:  %s *****\n", kbuf);
    printk(KERN_INFO "***** copyLength:  %lu *****\n", copyLength);

    // clear display

    // print on the first line by default
    glcd_printf(kbuf, 0, 0,0);
    zip_pixels();
    draw_fb();
    printk(KERN_INFO "glcd Driver: write()\n");
    return len;
}

static long glcd_ioctl(struct file *p_file, unsigned int ioctl_command, unsigned long arg)
{

    struct ioctl_mesg ioctl_arguments;

    printk(KERN_INFO "glcd Driver: ioctl\n");

    if (((const void *)arg) == NULL)
    {
        printk(KERN_DEBUG "ERR: Invalid argument for glcd IOCTL \n");
        return -EINVAL;
    }

    memset(ioctl_arguments.kbuf, '\0', sizeof(char) * MAX_BUF_LENGTH);

    // copy ioctl command argument from user space
    if (copy_from_user(&ioctl_arguments, (const void *)arg, sizeof(ioctl_arguments)))
    {
        printk(KERN_DEBUG "ERR: Failed to copy from user space buffer \n");
        return -EFAULT;
    }

    switch ((char)ioctl_command)
    {
    case IOCTL_CLEAR_DISPLAY:
        printk("st7920: ioctl_clear_display");
        clear_screen();
        break;

    case IOCTL_PRINT:
        printk("st7920: ioctl_print");
        glcd_printf(ioctl_arguments.kbuf, 0, 0,ioctl_arguments.invert);
        break;

    case IOCTL_PRINT_WITH_POSITION:
        printk("st7920: ioctl_print_w_position x=%i y=%i str=%s", ioctl_arguments.nthCharacter,
               ioctl_arguments.lineNumber, ioctl_arguments.kbuf);
        glcd_printf(ioctl_arguments.kbuf, ioctl_arguments.lineNumber, ioctl_arguments.nthCharacter,ioctl_arguments.invert);
        break;

    case IOCTL_PRINT_BMP:
        printk("st7920: ioctl_bmp");
        print_bitmap(ioctl_arguments.kbuf, 1042);
        break;

    case IOCTL_RESET:
        init_lcd();
        printk("st7920: ioctl_reset");
        break;

    case IOCTL_BACKLIGHT_ON:
        printk("ioctl_backlight_on");
        set_backlight(1);
        break;

    case IOCTL_BACKLIGHT_OFF:
        printk("ioctl_backlight_off");
        set_backlight(0);
    default:
        printk(KERN_DEBUG "glcd Driver (ioctl): %c No such command \n,ioctl_command");
        return -ENOTTY;
    }
    return 0;
}
/* This will create the init and exit function automatically */
module_spi_driver(graphic_display);

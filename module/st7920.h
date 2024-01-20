#include <linux/cdev.h>    // for struct cdev, obviously
#include <linux/spi/spi.h> // for struct spi_device

/* Declate the probe and remove functions */
static int glcd_probe(struct spi_device *client);
static void glcd_remove(struct spi_device *client);
/* Low level spi functions */
static void send_cmd(char cmd);
static int send_data(char data);
/* high level screen functions */
static void init_lcd(void); // <- you need to pass void explicity ( not as ())
static void screen_test(void);
static void clear_screen(void);
static void set_graphic_mode(void);
void print_bitmap(char *bmp, int size);
/* helper pixel functions  */
static void set_pixel(u8 x, u8 y, u8 set);
static void draw_fb(void);
static void zip_pixels(void);
static void alloc_char(char c, unsigned int x, unsigned int y);
/* helper syscall functions  */
static void glcd_printf(char *msg, unsigned int lineNumber, unsigned int nthCharacter);
static int glcd_open(struct inode *p_inode, struct file *p_file);
static int glcd_close(struct inode *p_inode, struct file *p_file);
static ssize_t glcd_write(struct file *p_file, const char __user *buf, size_t len, loff_t *off);
static long glcd_ioctl(struct file *p_file, unsigned int ioctl_command, unsigned long arg);
static ssize_t glcd_read(struct file *p_file, char __user *buf, size_t len, loff_t *off);
#define NUM_CHARS_PER_LINE 21 // the number of characters per line

// ********* Linux driver Constants ******************************************************************

#define MINOR_NUM_START 0 // minor number starts from 0
#define MINOR_NUM_COUNT 1 // the number of minor numbers required

#define MAX_BUF_LENGTH 1042 // maximum length of a buffer to copy from user space to kernel space

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

// ********* IOCTL COMMAND ARGUMENTS ******************************************************************

#define IOCTL_CLEAR_DISPLAY '0' // Identifiers for ioctl reqursts
#define IOCTL_PRINT '1'
#define IOCTL_PRINT_WITH_POSITION '3'
#define IOCTL_PRINT_BMP '4'
#define IOCTL_RESET '5'
#define IOCTL_BACKLIGHT_ON '6'
#define IOCTL_BACKLIGHT_OFF '7'

struct ioctl_mesg
{ // a structure to be passed to ioctl argument
    char kbuf[MAX_BUF_LENGTH];

    unsigned int lineNumber;
    unsigned int nthCharacter;
    unsigned int nbytes;
};

// ********* Device Structures *********************************************************************

#define CLASS_NAME "glcd"
#define DEVICE_NAME "glcd"

static dev_t dev_number;         // dynamically allocated device major number
struct cdev glcd_cdev;           // cdev structure
static struct class *glcd_class; // class structure

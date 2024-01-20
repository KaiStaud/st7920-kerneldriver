#include "CLI11.hpp"
#include "bmp-parser.hpp"
#include "emulator.hpp"
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>

#define MAX_BUF_LENGTH 1042
#define IOCTL_CLEAR_DISPLAY '0'
#define IOCTL_PRINT '1'
#define IOCTL_PRINT_WITH_POSITION '3'
#define IOCTL_PRINT_BMP '4'
#define IOCTL_RESET '5'
#define IOCTL_BACKLIGHT_ON '6'
#define IOCTL_BACKLIGHT_OFF '7'

struct ioctl_mesg
{
    char kbuf[MAX_BUF_LENGTH];
    unsigned int nthCharacter;
    unsigned int lineNumber;
    unsigned int nbytes;
};

int main(int argc, char *argv[])
{
    CLI::App app{"IOCTL Demo"};
    app.require_subcommand(1);

    int file = open("/dev/glcd", O_RDWR);
    if (file == -1)
    {
        perror("Error opening file");
        return 1;
    }

    struct ioctl_mesg message;
    memset(&message, 0, sizeof(struct ioctl_mesg));

    std::string inputString;
    bool printBmp = 0;
    bool reset = false;
    bool clear = false;
    bool backlightOn = false;
    bool backlightOff = false;
    bool printRaw = false;
    bool verbose = false;
    std::string bmpPath;
    std::string bufferPath;
    std::string eFile;
    int row, column;
    std::vector<char> bytes;
    bool readDisplay;
    std::string displayBin;
    /* Command Set */
    auto emulate_display = app.add_subcommand("emulate", "prints a pseudo display to stdout");
    emulate_display->add_option("--efile", eFile, "print to file instead");

    auto write_data = app.add_subcommand("data", "Write text/raw data");
    write_data->add_option("--string", inputString, "Write text to display");
    write_data->add_option("-x", message.lineNumber, "print @ row");
    write_data->add_option("-y", message.nthCharacter, "print @ column");
    write_data->add_option("--raw", bytes, "send raw bytes to display")->excludes("-x")->excludes("-y");

    auto write_command = app.add_subcommand("command", "Write commands");
    write_command->add_option("--bytes", bytes, "n bytes to write to device");
    write_command->add_flag("-r,--reset", reset, "reset device");
    write_command->add_flag("--backlight-on", backlightOn, "Set Backlight On(=1)");
    write_command->add_flag("--backlight-off", backlightOff, "Set Backlight Off(=0)");
    write_command->add_flag("--clear", clear, "clear screen");
    write_command->add_flag("--read-back", readDisplay, "readback display memory");
    write_command->add_option("--to-bin", displayBin, "read display into .bin file");

    write_command->require_option(1); // only one option!

    auto bitmap = app.add_subcommand("bmp", "Display Bitmap on Display");
    bitmap->add_option("--bmp-path", bmpPath, "Path for IOCTL_PRINT_BMP");
    bitmap->add_option("--print-buffer", bufferPath, "Write Screen Buffer to file");

    /* Callbacks */
    write_data->callback([&]() {
        if (!inputString.empty())
        {
            // IOCTL_PRINT
            strncpy(message.kbuf, inputString.c_str(), MAX_BUF_LENGTH);
            if ((message.lineNumber != 0) || (message.nthCharacter != 0))
            {
                printf("Printing %s on position %d/%d\r\n", message.kbuf, message.nthCharacter, message.lineNumber);
                ioctl(file, IOCTL_PRINT_WITH_POSITION, &message);
            }
            else
            {
                ioctl(file, IOCTL_PRINT, &message);
            }
        }
        if (!bytes.empty())
        {
            printf("Sending Raw data not implemented yet");
        }
    });

    write_command->callback([&]() {
        if (reset)
        {
            ioctl(file, IOCTL_RESET, &message);
        }
        if (backlightOn)
        {
            ioctl(file, IOCTL_BACKLIGHT_ON, &message);
        }
        if (backlightOff)
        {
            ioctl(file, IOCTL_BACKLIGHT_OFF, &message);
        }
        if (clear)
        {
            ioctl(file, IOCTL_CLEAR_DISPLAY, &message);
        }
    });

    bitmap->callback([&]() {
        /*
                for (int i = 0; i < 1024; i++)
                {
                    message.kbuf[i] = 0;
                    if (i <= 15)
                        message.kbuf[i] = 0xFF;
                    if ((i >= 1008) && (i <= 1024))
                        message.kbuf[i] = 0xFF;
                    // left boarder
                    if ((i % 16) == 0)
                        message.kbuf[i] = 0x80;
                }
                for (int i = 1; i <= 64; i++)
                {
                    message.kbuf[i * 16] = 0x80;
                    message.kbuf[i * 16 - 1] = 0x01;
                }
                message.kbuf[15] = 0xff;
                message.kbuf[0] = 0xff;
                message.kbuf[1023] = 0xff;
                message.kbuf[1008] = 0xff;
        */
        file_to_bytes(bmpPath, message.kbuf);
        message.nbytes = 1024;
        if (bufferPath != "")
        {
            int fd;
            const char *name = bufferPath.c_str();
            fd = open(name, O_WRONLY | O_CREAT, 0644);
            if (fd == -1)
            {
                perror("open failed");
                exit(1);
            }

            if (dup2(fd, 1) == -1)
            {
                perror("dup2 failed");
                exit(1);
            }
            printf("const unsigned char writeback[] = {");
            for (int i = 0; i < 1024; i++)
            {
                if (i % 16 == 0)
                {
                    printf("\r\n");
                }
                printf(" 0x%02x,", message.kbuf[i]);
            }
            printf("}");
        }
        ioctl(file, IOCTL_PRINT_BMP, &message);
    });

    emulate_display->callback([&]() {
        if (inputString != " ")
        {
            emulate_glcd_printf("hello world", message.nthCharacter, message.lineNumber);
        }
        if (bmpPath != " ")
        {
            file_to_bytes(bmpPath, message.kbuf);
            print_screen(message.kbuf);
        }
    });

    app.failure_message(CLI::FailureMessage::help);
    CLI11_PARSE(app, argc, argv);

    close(file);
    return 0;
}

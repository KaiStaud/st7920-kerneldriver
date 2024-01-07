#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
    // Check if the command-line argument is provided
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <string>\n", argv[0]);
        return 1;
    }

    FILE *file;
    char *content = argv[1];
    const char *filename = "/dev/glcd";

    // Open file for writing
    file = fopen(filename, "w");

    // Check if the file opened successfully
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    // Write the string to the file
    fputs(content, file);

    // Close the file
    fclose(file);



    return 0;
}

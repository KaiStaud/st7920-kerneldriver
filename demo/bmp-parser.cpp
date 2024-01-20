#include "bmp-parser.hpp"
#include <fcntl.h>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <sys/mman.h>
#include <unistd.h>
#include <vector>

void file_to_bytes(std::string path, char *bytes)
{
    // Regular expression pattern to match hex bytes without leading 0x
    std::regex pattern("([0-9a-fA-F]{2})");

    int fd = open(path.c_str(), O_RDONLY);
    int len = lseek(fd, 0, SEEK_END);
    void *data = mmap(0, len, PROT_READ, MAP_PRIVATE, fd, 0);
    std::string *sp = static_cast<std::string *>(data);
    printf("Read file with length: %d", len);
    char tinput[9000];
    sprintf(tinput, "%s", sp);
    std::string input = tinput;
    // Iterator for matching

    auto byteIterator = std::sregex_iterator(input.begin(), input.end(), pattern);
    auto byteEnd = std::sregex_iterator();

    int i = 0;
    // Iterate over matches
    while (byteIterator != byteEnd)
    {
        std::smatch match = *byteIterator;
        std::string byteStr = match[1].str(); // Group 1 contains the hex byte
        char byte = static_cast<char>(std::stoi(byteStr, nullptr, 16));
        bytes[i] = byte;
        i++;
        ++byteIterator;
    }
    printf("Decoded %i bytes!\r\n", i);
}

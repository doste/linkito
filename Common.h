#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include <vector>
#include <string>
#include <optional>
#include <map>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <mach-o/reloc.h>
#include <cerrno>

typedef uint8_t Byte;
enum macho_filetype {RelocatableObjectFile, ExecutableFile, DynamicLibrary};

// Source: https://www.mikeash.com/pyblog/friday-qa-2012-11-09-dyld-dynamic-linking-on-os-x.html
#define STANDARD_EXECUTABLE_LOAD_ADDR 0x0000000100000000

#define PAGE_SIZE 0x4000 // = 16384


uint64_t align_to(uint64_t val, uint64_t align);


void read_macho_header(FILE* fptr, struct mach_header_64* header);
FILE* open_macho_file(const char *pathname);

class File {
    public:
        File();
        File(char* filename, size_t filesize, FILE* fptr);
        static size_t get_file_size(FILE* fptr);
        void fill_buffer();

        char* filename;
        uint32_t filesize;
	    FILE* fptr;
	    uint8_t* buffer;
};

////////////////////////////////////////////////////////////////////////////////////////////


extern std::map<uint32_t, std::string> macroToString;

////////////////////////////////////////////////////////////////////////////////////////////


#endif
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
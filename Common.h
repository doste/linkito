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

struct MachHeader64 {
	uint32_t	    magic;		        /* mach magic number identifier */
	cpu_type_t	    cputype;	        /* cpu specifier */
	cpu_subtype_t	cpusubtype;	        /* machine specifier */
	uint32_t	    filetype;	        /* type of file */
	uint32_t	    ncmds;		        /* number of load commands */
	uint32_t	    sizeofcmds;	        /* the size of all the load commands */
	uint32_t	    flags;		        /* flags */
	uint32_t	    reserved;	        /* reserved */
};



// Source: https://www.mikeash.com/pyblog/friday-qa-2012-11-09-dyld-dynamic-linking-on-os-x.html
#define STANDARD_EXECUTABLE_LOAD_ADDR 0x0000000100000000

#define PAGE_SIZE 0x4000 // = 16384

#define LIB_SYSTEM_PATH_NAME  "/usr/lib/libSystem.B.dylib"
#define DYLD_PATH_NAME        "/usr/lib/dyld"


uint64_t align_to(uint64_t val, uint64_t align);


void read_macho_header(FILE* fptr, MachHeader64* header);
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


const std::vector<uint32_t> getLinkeditCommands();
bool isLinkeditDataCommand(uint32_t input_cmd);


size_t alignStringLengthToSixteen(char* a_string);
char* allocMemoryForPathnameAligned(char* pathname, size_t pathname_size_aligned);


#endif
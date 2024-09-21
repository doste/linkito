#ifndef __MACHO_PARSER_H__
#define __MACHO_PARSER_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <mach-o/loader.h>
#include <sys/_types/_size_t.h>
#include <mach-o/nlist.h>
#include <mach-o/reloc.h>

#define LENGHT_NAME 16 // loader.h defines it this way ::shrug::

struct SymbolHelper {
    char* name;
    uint64_t addr;
    int size;
    //enum Type type;
    char* type;
    uint8_t section_number;     //  (where this symbol is defined)
   // enum Binding bind;
};

/*
struct SectionHandler {
	char sectname[16];	// name of this section
	char segname[16];	// segment this section goes in

    size_t number_of_section;
};
*/
struct SegmentHandle;

//  *** Section ***
struct SectionHandle {
    char* section_name;
    struct section_64 section_cmd;
    uint8_t* raw_data_of_section;
    struct SegmentHandle* segment;
};


struct VectorOfSections {
	struct SectionHandle** sections;
    size_t size;
    size_t capacity;
};
struct VectorOfSections* new_vector_of_sections(void);
void push_section(struct VectorOfSections* vector_of_sections, struct SectionHandle* section_handle);
struct SectionHandle* get_section_handle_at(struct VectorOfSections* vector_of_sections, size_t idx);
size_t get_size_of_vector_of_sections(struct VectorOfSections* vector_of_sections);

//  *** Segment ***
struct SegmentHandle {
    char* segment_name;
    struct segment_command_64 load_cmd;
    struct VectorOfSections* sections;
};

struct VectorOfSegments {
	struct SegmentHandle** segments;
    size_t size;
    size_t capacity;
};
struct VectorOfSegments* new_vector_of_segments(void);
void push_segment(struct VectorOfSegments* vector_of_segments, struct SegmentHandle* segment_handle);
struct SegmentHandle* get_segment_handle_at(struct VectorOfSegments* vector_of_segments, size_t idx);
size_t get_size_of_vector_of_segments(struct VectorOfSegments* vector_of_segments);


////
struct SymbolsHandle {
    struct SymbolHelper* symbols;
    size_t number_of_symbols;
};

typedef enum {RelocatableObjectFile, ExecutableFile, DynamicLibrary} macho_filetype;

struct InputFile {
    char* filename;
    size_t filesize;
	FILE* fptr;
	uint8_t* buffer;
};

// Each MachoHandle will corresponds to each Mach-o file we are processing
struct MachoHandle {
    FILE* fptr;
    struct mach_header_64 header;
    macho_filetype filetype;
    struct InputFile* input_file;


    void* load_commands;

    //struct SymbolsHandle* symbols_handle;
    //struct VectorOfSections* sections;                      //TODO: remove this
    //struct SectionHelper* sections_helper;
    struct VectorOfSegments* segments;
};


FILE* open_macho_file(const char *pathname, const char *mode);


void get_cmd_and_cmdsize_of_load_command(struct MachoHandle* handle, uint32_t* cmd, uint32_t* cmdsize, size_t* offset);

//char* build_string_table(struct MachoHandle* handle, struct symtab_command* symtab);
//void build_symbols_handler(struct MachoHandle* handle, struct symtab_command* symtab);
//struct symtab_command* read_symtab(struct MachoHandle* handle);

void read_segment(struct MachoHandle* handle, struct segment_command_64* segment_cmd, size_t* offset_to_read_segment);
void read_section(struct MachoHandle* handle, struct section_64* section_cmd, size_t* offset_to_read_section);

//uint8_t* get_raw_data_from_section(struct MachoHandle* handle, struct section_64* section);
//struct SectionHelper** get_raw_data_from_sections(struct MachoHandle* handle);

struct MachoHandle* build_macho_handle(FILE* fptr, char* filename);



///////////// helpers . TODO: extract to separate files /////////////

void read_struct_from_file(FILE* fptr, void* struct_itself, size_t size_of_struct, long offset_to_start_reading, bool restore_fptr);
void get_objc_classes(struct MachoHandle* handle);
//void get_relocation_entries(struct MachoHandle* handle);

#endif
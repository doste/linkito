
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <mach-o/loader.h>
#include <sys/_types/_size_t.h>
#include <mach-o/nlist.h>
#include <mach-o/reloc.h>
#include <sys/errno.h>
#include "../include/macho_parser.h"
#include "../include/debug.h"
#include "../include/helpers.h"

//  *** Section ***
struct VectorOfSections* new_vector_of_sections(void) {
    struct VectorOfSections* vec = malloc(sizeof(struct VectorOfSections));
    vec->size = 0;                                                                                                                                            
    vec->capacity = 4;
    vec->sections = malloc(vec->capacity * sizeof(struct SectionHandle*));
    return vec;
}

struct VectorOfSections* new_vector_of_sections_of_size(size_t size) {
    struct VectorOfSections* vec = malloc(sizeof(struct VectorOfSections));
    vec->size = 0;                                                                                                                                            
    vec->capacity = size;
    vec->sections = malloc(vec->capacity * sizeof(struct SectionHandle*));
    return vec;
 }


void push_section(struct VectorOfSections* vector_of_sections, struct SectionHandle* section_handle) {

    if (vector_of_sections->size >= vector_of_sections->capacity) {
        size_t new_capacity = 2 * vector_of_sections->capacity;
        vector_of_sections->sections = realloc(vector_of_sections->sections, new_capacity * sizeof(struct SectionHandle*));
        vector_of_sections->capacity = new_capacity;
    }
    vector_of_sections->sections[vector_of_sections->size] = section_handle;
    vector_of_sections->size++;
}

size_t get_size_of_vector_of_sections(struct VectorOfSections* vector_of_sections) {
    return vector_of_sections->size;
}

struct SectionHandle* get_section_handle_at(struct VectorOfSections* vector_of_sections, size_t idx) {
    if (idx >= get_size_of_vector_of_sections(vector_of_sections)) {
        return NULL;
    }
    return vector_of_sections->sections[idx];
}

struct SectionHandle* get_section_handle_with_section_name(struct VectorOfSections* vector_of_sections, char* section_name) {
    for (size_t i = 0; i < get_size_of_vector_of_sections(vector_of_sections); i++) {
        struct SectionHandle* sect_handle = get_section_handle_at(vector_of_sections, i);
        if (strcmp(sect_handle->section_name, section_name) == 0) {
            return sect_handle;
        }
    }
    return NULL;
}



//  *** Segment ***
struct VectorOfSegments* new_vector_of_segments(void) {
    struct VectorOfSegments* vec = malloc(sizeof(struct VectorOfSegments));
    vec->size = 0;                                                                      // this size is going to be used as index. Specifically as index in the array of sections for
                                                                                        // a segment, and this array starts at 1. So we initiliaze size with 1.
    vec->capacity = 4;
    vec->segments = malloc(vec->capacity * sizeof(struct SegmentHandle*));
    return vec;
}
void push_segment(struct VectorOfSegments* vector_of_segments, struct SegmentHandle* segment_handle) {

    if (vector_of_segments->size >= vector_of_segments->capacity) {
        size_t new_capacity = 2 * vector_of_segments->capacity;
        vector_of_segments->segments = realloc(vector_of_segments->segments, new_capacity * sizeof(struct SegmentHandle*));
        vector_of_segments->capacity = new_capacity;
    }
    vector_of_segments->segments[vector_of_segments->size] = segment_handle;
    vector_of_segments->size++;

}

size_t get_size_of_vector_of_segments(struct VectorOfSegments* vector_of_segments) {
    return vector_of_segments->size;
}

struct SegmentHandle* get_segment_handle_at(struct VectorOfSegments* vector_of_segments, size_t idx) {
    if (idx >= get_size_of_vector_of_segments(vector_of_segments)) {
        return NULL;
    }
    return vector_of_segments->segments[idx];
}

struct SegmentHandle* get_segment_handle_with_segment_name(struct VectorOfSegments* vector_of_segments, char* segment_name) {
    for (size_t i = 0; i < get_size_of_vector_of_segments(vector_of_segments); i++) {
        struct SegmentHandle* seg_handle = get_segment_handle_at(vector_of_segments, i);
        if (strcmp(seg_handle->segment_name, segment_name) == 0) {
            return seg_handle;
        }
    }
    return NULL;
}

/////

FILE* open_macho_file(const char *pathname, const char *mode) {
    FILE*  fptr = fopen(pathname, mode);
    if (fptr == NULL) {
        fprintf(stderr, "Could not open: %s. %s\n", pathname, strerror(errno));
    }
    assert(fptr != NULL);
    return fptr;
}

void get_cmd_and_cmdsize_of_load_command(struct MachoHandle* macho_handle, uint32_t* cmd,
                                            uint32_t* cmdsize, size_t* offset_to_read_from_buffer) {
    // Read the first 8 bytes to get cmd and cmdsize
    //struct load_command {
	//uint32_t cmd;		/* type of load command */
	//uint32_t cmdsize;	/* total size of command in bytes */
    uint8_t* buffer = macho_handle->input_file->buffer;
    memcpy(cmd, buffer + *offset_to_read_from_buffer, sizeof(uint32_t));
    *offset_to_read_from_buffer += sizeof(uint32_t);
    memcpy(cmdsize, buffer + *offset_to_read_from_buffer, sizeof(uint32_t));
    *offset_to_read_from_buffer += sizeof(uint32_t);
}


/*
char* build_string_table(struct MachoHandle* handle, struct symtab_command* symtab) {
    // We'll move the fptr so we need to save its value
    ////long saved_fptr = ftell(handle->fptr);

     // Move the fptr to the offset corresponding to the string table
    ////fseek(handle->fptr, symtab->stroff, SEEK_SET);

    char* buffer_of_strings = malloc(symtab->strsize);
    ////size_t items_read = 0;
    ////items_read = fread(buffer_of_strings, symtab->strsize, 1, handle->fptr);
    ////assert(items_read == 1);

    read_struct_from_file(handle->fptr, buffer_of_strings, symtab->strsize, symtab->stroff, true);

    // Restore the value of the fptr
    ////if (fseek(handle->fptr, saved_fptr, SEEK_SET) != 0) printf("fseek error\n");

    return buffer_of_strings;
}

void build_symbols_handler(struct MachoHandle* handle, struct symtab_command* symtab) {
    struct SymbolsHandle* sym_handle = malloc(sizeof(struct SymbolsHandle));

    char* string_table = build_string_table(handle, symtab);

    // How many symbols do we have?
    uint32_t number_of_symbols = symtab->nsyms;

    // Build the struct SymbolsHandle:
    sym_handle->symbols = malloc(sizeof(struct SymbolHelper) * number_of_symbols);
    sym_handle->number_of_symbols = number_of_symbols;

    // We'll need to access to the array of nlist structs containing the data of the symbols. To do that we use the offset given by the symtab_command
    // We'll move the fptr so we need to save its value
    long saved_fptr = ftell(handle->fptr);
    // Move the fptr to the corresponding offset
    fseek(handle->fptr, symtab->symoff, SEEK_SET);

    // For each symbol, we'll want to build an struct SymbolHelper and put it in the sym_handle->symbols array.
    for (uint32_t i = 0; i < number_of_symbols; i++) {
        struct SymbolHelper sym_helper = {0};

        struct nlist_64 nlist = {0};
        ////size_t items_read = 0;
        ////items_read = fread(&nlist, sizeof(struct nlist_64), 1, handle->fptr);
        ////assert(items_read == 1);

        read_struct_from_file(handle->fptr, &nlist, sizeof(struct nlist_64), 0, false);

        char* type = NULL;
        //uint8_t* section_number = NULL;

        switch(nlist.n_type & N_TYPE) {
            case N_UNDF: type = "N_UNDF"; break;    // undefined, n_sect == NO_SECT (0)
            case N_ABS:  type = "N_ABS";  break;    // absolute, n_sect == NO_SECT  (0)
            case N_SECT: type = "N_SECT"; break;    // defined in section number n_sect
            case N_PBUD: type = "N_PBUD"; break;    // prebound undefined (defined in a dylib)
            case N_INDR: type = "N_INDR"; break;    // indirect
            default:
                fprintf(stderr, "Invalid symbol type: 0x%x\n", nlist.n_type & N_TYPE);
                exit(1);
        }

        sym_helper.name = string_table + nlist.n_un.n_strx;
        sym_helper.addr = nlist.n_value;
        sym_helper.type = type;
        sym_helper.section_number = nlist.n_sect;

        sym_handle->symbols[i] = sym_helper;
    }

    // Restore the value of the fptr
    if (fseek(handle->fptr, saved_fptr, SEEK_SET) != 0) printf("fseek error\n");

    // Save the newly built struct SymbolsHandle to the struct MachoHandle:
    //handle->symbols_handle = sym_handle;
}
*/
// struct symtab_command is defined in <mach-o/loader.h>
/*
struct symtab_command {
	uint32_t	cmd;		// LC_SYMTAB
	uint32_t	cmdsize;	// sizeof(struct symtab_command)
	uint32_t	symoff;		// symbol table offset                          -> offset from the start of the file to an array of nlist structures containing data on each symbol
	uint32_t	nsyms;		// number of symbol table entries               -> number of symbols (or nlist structs)
	uint32_t	stroff;		// string table offset                          -> file offset to the strings used by the symbol lookup
	uint32_t	strsize;	// string table size in bytes
};
struct nlist_64 is defined in <mach-o/nlist.h>         . This struct gives us information about a named symbol.
struct nlist_64 {
    union {
        uint32_t  n_strx;  // index into the string table                   -> offset from the symbol string field to the string of this symbol
    } n_un;
    uint8_t n_type;        // type flag, see below
    uint8_t n_sect;        // section number or NO_SECT
    uint16_t n_desc;       // see <mach-o/stab.h>
    uint64_t n_value;      // value of this symbol (or stab offset)         -> contains the value of the symbol, such as the address.
};

struct symtab_command* read_symtab(struct MachoHandle* handle) {
    // We'll move the fptr so we need to save its value
    ////long saved_fptr = ftell(handle->fptr);

    // No need to set the fptr here because the caller of this function (build_macho_handle) already set it.
    // So, a Precondition to call read_symtab is that the handle->fptr is pointing to the symtab_command in the file.

    struct symtab_command* symtab_load_cmd = malloc(sizeof(struct symtab_command));
    ////size_t items_read = 0;
    ////items_read = fread(symtab_load_cmd, sizeof(struct symtab_command), 1, handle->fptr);
    ////assert(items_read == 1);

    read_struct_from_file(handle->fptr, symtab_load_cmd, sizeof(struct symtab_command), 0, true);

    // Restore the value of the fptr
    ////if (fseek(handle->fptr, saved_fptr, SEEK_SET) != 0) printf("fseek error\n");

    return symtab_load_cmd;
}
*/

void read_segment(struct MachoHandle* macho_handle, struct segment_command_64* segment_cmd, size_t* offset_to_read_segment) {
    memcpy(segment_cmd, macho_handle->input_file->buffer + *offset_to_read_segment, sizeof(struct segment_command_64));
    *offset_to_read_segment += sizeof(struct segment_command_64);
}


void read_section(struct MachoHandle* macho_handle, struct section_64* section_cmd, size_t* offset_to_read_section) {
    memcpy(section_cmd, macho_handle->input_file->buffer + *offset_to_read_section, sizeof(struct section_64));
    *offset_to_read_section += sizeof(struct section_64);
}

struct SegmentHandle* build_segment_handle(struct MachoHandle* macho_handle, size_t* offset_to_read_segment) {
    // build_segment_handle is always called when reading a segment, when reading all load commands,
    // if the load command happens to be a LC_SEGMENT then this function will be called.
    // But in that case, we are already determined that the command was in fact a LC_SEGMENT, so that means
    // we are read the cmd and cmdsize fields of the corresponding struct segment_command_64
    // To account for that we subtract this amount (both cmd and cmdsize are uint32_t)to the offset_to_read_segment parameter,
    // to read the struct correctly.
    *offset_to_read_segment -= (sizeof(uint32_t) + sizeof(uint32_t));
    struct SegmentHandle* segment_handle = malloc(sizeof(struct SegmentHandle));
    struct segment_command_64 load_cmd;
    read_segment(macho_handle, &load_cmd, offset_to_read_segment);
    segment_handle->segment_name = malloc(sizeof(uint8_t) * LENGHT_NAME);
    strcpy(segment_handle->segment_name, load_cmd.segname);
    segment_handle->load_cmd = load_cmd;
    segment_handle->sections = new_vector_of_sections();
    return segment_handle;
}


struct SectionHandle* build_section_handle(struct MachoHandle* macho_handle,
                                            struct SegmentHandle* segment_handle, size_t* offset_to_read_section) {
    struct SectionHandle* section_handle = malloc(sizeof(struct SectionHandle));
    struct section_64 section_cmd;
    read_section(macho_handle, &section_cmd, offset_to_read_section);
    section_handle->section_name = malloc(sizeof(uint8_t) * LENGHT_NAME);
    strcpy(section_handle->section_name, section_cmd.sectname);
    section_handle->section_cmd = section_cmd;
    section_handle->segment = segment_handle;

    // TODO: initialize raw_data pointer ???
    return section_handle;
}



struct MachoHandle* build_macho_handle(FILE* fptr, char* filename) {
    struct MachoHandle* macho_handle = (struct MachoHandle*)malloc(sizeof(struct MachoHandle));

    macho_handle->fptr = fptr;
    macho_handle->segments = new_vector_of_segments();

    macho_handle->input_file = malloc(sizeof(struct InputFile));
    macho_handle->input_file->fptr = macho_handle->fptr;
    macho_handle->input_file->filename = calloc(strlen(filename), sizeof(uint8_t));
	strcpy(macho_handle->input_file->filename, filename);
    macho_handle->input_file->filesize = get_input_file_size(macho_handle->fptr);
    macho_handle->input_file->buffer = calloc(macho_handle->input_file->filesize, sizeof(uint8_t));

    // We'll move the fptr so we need to save its value
    long saved_fptr = ftell(macho_handle->fptr);
    size_t items_read = fread(macho_handle->input_file->buffer, sizeof(uint8_t), macho_handle->input_file->filesize, macho_handle->fptr );
    if (items_read != macho_handle->input_file->filesize) {
        printf("fread failed while reading the entire file.\n");
        exit(1);
    }
    // Restore the value of the fptr
    if (fseek(macho_handle->fptr, saved_fptr, SEEK_SET) != 0) printf("fseek error\n");

    //items_read = fread(&(macho_handle->header), sizeof(struct mach_header_64), 1, macho_handle->fptr );
    //if (items_read != 1) {
    //    printf("fread failed while reading the mach header.\n");
    //    exit(1);
    //}
    if (fseek(macho_handle->fptr, sizeof(struct mach_header_64), SEEK_SET) != 0) printf("fseek error\n");

    uint8_t* buffer = macho_handle->input_file->buffer;
    size_t offset_to_read_from_buffer = 0;
    memcpy(&macho_handle->header, buffer+offset_to_read_from_buffer, sizeof(struct mach_header_64));
    offset_to_read_from_buffer += sizeof(struct mach_header_64);

    switch (macho_handle->header.filetype) {
        case MH_OBJECT:
            macho_handle->filetype = RelocatableObjectFile;
            break;
        case MH_EXECUTE:
            macho_handle->filetype = ExecutableFile;
            break;
        case MH_DYLIB:
            macho_handle->filetype = DynamicLibrary;
            break;
        default:
            printf("Not supported filetype\n");
            exit(1);
    }

    macho_handle->load_commands = malloc(macho_handle->header.sizeofcmds);

    // loader.h -> "The load commands directly follow the mach_header[...]"
    for (size_t i = 0; i < macho_handle->header.ncmds; i++) {
        uint32_t cmd, cmdsize;
        get_cmd_and_cmdsize_of_load_command(macho_handle, &cmd, &cmdsize, &offset_to_read_from_buffer);
        #if DEBUG
        print_command_type(cmd);
        printf("cmdsize: %d\n\n", cmdsize);
        #endif
        switch (cmd) {
            case LC_SYMTAB:
            {
                //struct symtab_command* symtab = read_symtab(handle);
                //build_symbols_handler(handle, symtab);
                //free(symtab);
                offset_to_read_from_buffer += cmdsize;
                offset_to_read_from_buffer -= (sizeof(uint32_t) + sizeof(uint32_t));  // Subtract that we are already read the cmd and 
                                                                                      // cmdsize fields so the offset was left advanced 
                break;
            }
            case LC_SEGMENT_64:
            {
                long saved_fptr = ftell(fptr);
                
                struct SegmentHandle* segment_handle = build_segment_handle(macho_handle, &offset_to_read_from_buffer);
                push_segment(macho_handle->segments, segment_handle);
                // Right after the struct segment_command_64 there is the array containing all the sections defined in this segment. The number of them is given by:
                // segment->nsects . Each entry of that array will be of type struct section_64. In there, each section will have its own size, so we'll read them using that size
                // to move forward the fptr accordingly.

                // We read all the struct segment_command_64, so the fptr is pointing to the end of the struct, exactly where the sections begin.
                for (size_t j = 0; j < segment_handle->load_cmd.nsects; j++) {

                    struct SectionHandle* section_handle = build_section_handle(macho_handle, segment_handle, &offset_to_read_from_buffer);
                    push_section(segment_handle->sections, section_handle);

                    
                }
                //if (fseek(macho_handle->fptr, saved_fptr, SEEK_SET) != 0) printf("fseek error\n");
                break;
            }
            case LC_BUILD_VERSION:
            {
                offset_to_read_from_buffer += cmdsize; // Because after the struct itself it may have other structs,
                                                       // loader.h: " The list of known platforms and tool values following it."
                
                offset_to_read_from_buffer -= (sizeof(uint32_t) + sizeof(uint32_t));
                break;
            }
            case LC_DYSYMTAB:
            {
                offset_to_read_from_buffer += cmdsize;
                offset_to_read_from_buffer -= (sizeof(uint32_t) + sizeof(uint32_t));
                break;
            }
            default:
            {
                printf("Unknown cmd %d\n", cmd);
            }
        }

        // loader.h -> "To advance to the next load command the cmdsize can be added to the offset or pointer of the current load command."
    }

    return macho_handle;
}


void assign_raw_data_to_each_section(struct MachoHandle* macho_handle) {

    for (size_t i = 0; i < get_size_of_vector_of_segments(macho_handle->segments); i++) {

        struct SegmentHandle* segment_handle = get_segment_handle_at(macho_handle->segments, i);

        for (size_t j = 0; j < get_size_of_vector_of_sections(segment_handle->sections); j++) {
            
            uint8_t* buffer = macho_handle->input_file->buffer;
            struct SectionHandle* section_handle = get_section_handle_at(segment_handle->sections, j);
            section_handle->raw_data_of_section = malloc(sizeof(uint8_t) * section_handle->section_cmd.size);
            // read from the file starting at section_handle->section_cmd.offset
            memcpy(section_handle->raw_data_of_section,
                            buffer + section_handle->section_cmd.offset,
                            section_handle->section_cmd.size);
        }
    }
}

/*

///////////// helpers . TODO: extract to separate files /////////////


void get_objc_classes(struct MachoHandle* handle) {

    struct VectorOfSections* sections = handle->sections;

    printf("OBJC:\n");
    for (size_t i = 1; i < handle->sections->size; i++) {

        struct section_64* sect = handle->sections->sections[i];
        if (strcmp(sect->sectname, "__objc_classlist__DATA_CONST") == 0) {
           // printf("size: %llu\n", sect->size);
           // printf("nreloc: %d\n", sect->nreloc);

            //print_section_type(sect);
            // This __DATA_CONST.__objc_classlist section is an array of pointers to Objective-C classes

            //Class some_objc_class;
            //read_struct_from_file(handle->fptr,some_objc_class, sect->size, sect->offset, true);
            //printf("PTRRR: %p\n", some_objc_class);
            //cls_msg(some_objc_class, sel("someClassMethodito:"));
        }

        if (strcmp(sect->sectname, "__objc_classname__TEXT") == 0) {
            print_section_type(sect);
            char class_name[64];
            read_struct_from_file(handle->fptr,class_name, sect->size, sect->offset, true);
            printf("CLASS NAME: %s\n", class_name);
        }

        if (strcmp(sect->sectname, "__objc_methname") == 0) {
            print_section_type(sect);
            char method_name[64];
            read_struct_from_file(handle->fptr, method_name, sect->size, sect->offset, true);
            printf("METHOD NAME: %s\n", method_name);
        }
    }
}
*/





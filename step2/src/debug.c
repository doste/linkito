#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <sys/errno.h>
#include "../include/debug.h"

void print_symbol(struct MachoHandle* macho_handle)
{
    //printf("%s\n", symbol_helper->name);
    //printf("    type: %s\n", symbol_helper->type);
    //printf("    addr: %p\n", (void*)symbol_helper->addr);
    //printf("    section number: %d\n", symbol_helper->section_number);

    //if (symbol_helper->section_number != 0) {
    //    printf("    Section name: %s\n", handle->sections->sections[symbol_helper->section_number]->sectname);
    //    printf("    Segment name: %s\n", handle->sections->sections[symbol_helper->section_number]->segname);
    //}
}

void print_symbols(struct MachoHandle* macho_handle) {

    //struct SymbolsHandle* sym_handle = handle->symbols_handle;

    printf("Symbols:\n");

    //for (size_t i = 0; i < sym_handle->number_of_symbols; i++) {
    //    print_symbol(handle, &sym_handle->symbols[i]);
    //}
}

void print_section_type(struct section_64* sect) {
    printf("    Section type: ");
    switch(sect->flags & SECTION_TYPE) {
        case S_REGULAR:
            printf("S_REGULAR\n");
            break;
        case S_ZEROFILL:
            printf("S_ZEROFILL\n");
            break;
        case S_CSTRING_LITERALS:
            printf("S_CSTRING_LITERALS\n");
            break;
        case S_4BYTE_LITERALS:
            printf("S_4BYTE_LITERALS\n");
            break;
        case S_8BYTE_LITERALS:
            printf("S_8BYTE_LITERALS\n");
            break;
        case S_LITERAL_POINTERS:
            printf("S_LITERAL_POINTERS\n");
            break;
    }
}

void print_segments_and_sections(struct MachoHandle* macho_handle) {

    size_t number_of_segments = get_size_of_vector_of_segments(macho_handle->segments);
    printf("This file has %zu segments:\n", number_of_segments);

    for (size_t i = 0; i < number_of_segments; i++) {
        struct SegmentHandle* segment_handle = get_segment_handle_at(macho_handle->segments, i);
        if (!segment_handle) {
            printf("Error getting the segment handler at index: %zu\n", i);
            break;
        }
        if (strcmp(segment_handle->segment_name, "") == 0) {
            printf("Segment %zu has no name\n", i);
        } else {
            printf("Segment %zu name: %s\n", i, segment_handle->segment_name);
        }
        
        size_t number_of_sections = get_size_of_vector_of_sections(segment_handle->sections);
        printf("Segment %zu has %zu sections\n", i, number_of_sections);

        for (size_t j = 0; j < get_size_of_vector_of_sections(segment_handle->sections); j++) {
            struct SectionHandle* section_handle = get_section_handle_at(segment_handle->sections, j);
            if (!section_handle) {
                printf("Error getting the section handler at index: %zu in segment at index: %zu\n", j, i);
                break;
            }
            printf("    Section name: %s\n", section_handle->section_name);
            print_section_type(&(section_handle->section_cmd));

            printf("\n");
        }
    }
}



void print_file_type(struct MachoHandle* handle) {
    switch (handle->header.filetype) {
        case MH_OBJECT:
            printf("Relocatable object file\n");
            break;
        case MH_EXECUTE:
            printf("Executable file\n");
            break;
        default:
            printf("Not supported\n");
            break;
    }
}

void print_command_type(uint32_t cmd) {
    switch (cmd) {
        case LC_SEGMENT:
            printf("segment of this file to be mapped \n");
            break;
        case LC_SYMTAB:
            printf("Symtab\n");
            break;
        case LC_SEGMENT_64:
            printf("64-bit segment of this file to be mapped\n");
            break;
        case LC_LOAD_DYLIB:
            printf("load a dynamically linked shared library\n");
            break;
        case LC_FUNCTION_STARTS:
            printf("compressed table of function start addresses\n");
            break;
        default:
            printf("Unknown\n");
            break;
    }
}

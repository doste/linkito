#include <stdint.h>
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



// The largest value is LC_BUILD_VERSION = 0x32 *but* there are some (fortunally few) that are values with masks applied
// so for those values we need to handle them manually (are the ones commented out in the array definition)
char* macro_to_string[0x33] = {
                            [LC_SEGMENT_64] = "LC_SEGMENT_64",
                            [LC_ROUTINES_64] = "LC_ROUTINES_64",
                            //[LC_LOAD_WEAK_DYLIB] = "LC_LOAD_WEAK_DYLIB",
                            [LC_UUID] = "LC_UUID",
                            //[LC_RPATH] = "LC_RPATH ",
                            //[LC_REQ_DYLD] = "LC_REQ_DYLD ",
                            [LC_CODE_SIGNATURE] = "LC_CODE_SIGNATURE",
                            [LC_SEGMENT_SPLIT_INFO] = "LC_SEGMENT_SPLIT_INFO",
                            //[LC_REEXPORT_DYLIB] = "LC_REEXPORT_DYLIB ",
                            [LC_LAZY_LOAD_DYLIB ] = "LC_LAZY_LOAD_DYLIB ",
                            [LC_ENCRYPTION_INFO ] = "LC_ENCRYPTION_INFO ",
                            [LC_DYLD_INFO ] = "LC_DYLD_INFO ",
                            //[LC_DYLD_INFO_ONLY ] = "LC_DYLD_INFO_ONLY ",
                            //[LC_LOAD_UPWARD_DYLIB ] = "LC_LOAD_UPWARD_DYLIB ",
                            [LC_VERSION_MIN_MACOSX] = "LC_VERSION_MIN_MACOSX",
                            [LC_VERSION_MIN_IPHONEOS ] = "LC_VERSION_MIN_IPHONEOS ",
                            [LC_FUNCTION_STARTS] = "LC_FUNCTION_STARTS",
                            [LC_DYLD_ENVIRONMENT] = "LC_DYLD_ENVIRONMENT",
                            //[LC_MAIN] = "LC_MAIN",
                            [LC_DATA_IN_CODE] = "LC_DATA_IN_CODE",
                            [LC_SOURCE_VERSION] = "LC_SOURCE_VERSION",
                            [LC_DYLIB_CODE_SIGN_DRS] = "LC_DYLIB_CODE_SIGN_DRS",
                            [LC_ENCRYPTION_INFO_64] = "LC_ENCRYPTION_INFO_64",
                            [LC_LINKER_OPTION] = "LC_LINKER_OPTION",
                            [LC_LINKER_OPTIMIZATION_HINT] = "LC_LINKER_OPTIMIZATION_HINT",
                            [LC_VERSION_MIN_TVOS] = "LC_VERSION_MIN_TVOS",
                            [LC_VERSION_MIN_WATCHOS] = "LC_VERSION_MIN_WATCHOS",
                            [LC_NOTE] = "LC_NOTE",
                            [LC_BUILD_VERSION] = "LC_BUILD_VERSION",
                            [LC_SEGMENT] = "LC_SEGMENT",
                            [LC_SYMTAB] = "LC_SYMTAB",
                            [LC_SYMSEG	] = "LC_SYMSEG	",
                            [LC_THREAD] = "LC_THREAD",
                            [LC_UNIXTHREAD] = "LC_UNIXTHREAD",
                            [LC_LOADFVMLIB] = "LC_LOADFVMLIB",
                            [LC_IDFVMLIB] = "LC_IDFVMLIB",
                            [LC_IDENT] = "LC_IDENT",
                            [LC_FVMFILE] = "LC_FVMFILE",
                            [LC_PREPAGE] = "LC_PREPAGE",
                            [LC_DYSYMTAB] = "LC_DYSYMTAB",
                            [LC_LOAD_DYLIB] = "LC_LOAD_DYLIB",
                            [LC_ID_DYLIB] = "LC_ID_DYLIB",
                            [LC_LOAD_DYLINKER] = "LC_LOAD_DYLINKER",
                            [LC_ID_DYLINKER] = "LC_ID_DYLINKER",
                            [LC_PREBOUND_DYLIB] = "LC_PREBOUND_DYLIB",
                            [LC_ROUTINES] = "LC_ROUTINES",
                            [LC_SUB_FRAMEWORK] = "LC_SUB_FRAMEWORK",
                            [LC_SUB_UMBRELLA] = "LC_SUB_UMBRELLA",
                            [LC_SUB_CLIENT] = "LC_SUB_CLIENT",
                            [LC_SUB_LIBRARY] = "LC_SUB_LIBRARY",
                            [LC_TWOLEVEL_HINTS] = "LC_TWOLEVEL_HINTS",
                            [LC_PREBIND_CKSUM] = "LC_PREBIND_CKSUM",
                            };
// This array was automatically generated in Pharo, just with this code:
/*
    '''
    | pair arrayOfPairs megaString |
    megaString := String new.
    arrayOfPairs := OrderedCollection new.
    aReallyLongString do: [ :each | pair := '[', each, '] = "', each, '",', String cr . arrayOfPairs add: pair].
    arrayOfPairs collect: [ :e |  megaString := megaString, e].
    '''
    Where aReallyLongString was just generated by copypasting the macros from loader.h, like:
    'LC_ROUTINES_64	
    LC_UUID		
    LC_RPATH       
    LC_CODE_SIGNATURE   
    etc etc '

    Pharo is awesome :D
*/

void print_command_type(uint32_t cmd) {

    switch (cmd) {
        case LC_RPATH:
            printf("LC_RPATH\n");
            break;
        case LC_REEXPORT_DYLIB:
            printf("LC_REEXPORT_DYLIB\n");
            break;
        case LC_MAIN:
            printf("LC_MAIN\n");
            break;
        case LC_DYLD_INFO_ONLY:
            printf("LC_DYLD_INFO_ONLY\n");
            break;
        case LC_LOAD_UPWARD_DYLIB:
            printf("LC_LOAD_UPWARD_DYLIB\n");
            break;
        case LC_LOAD_WEAK_DYLIB:
            printf("LC_LOAD_WEAK_DYLIB\n");
            break;
        case LC_REQ_DYLD:
            printf("LC_REQ_DYLD\n");
            break;
        default:
            if (cmd > 0x32) {
                printf("Invalid cmd\n");
            } else {
                printf("%s\n", macro_to_string[cmd]);
            }
            break;
    }
}

void print_load_commands(struct MachoHandle* macho_handle) {
    printf("The file %s has %d load commands\n", macho_handle->input_file->filename, macho_handle->header.ncmds);
}


#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h>
#include <mach-o/loader.h>
#include <sys/_types/_size_t.h>
#include "../include/macho_parser.h"

void print_symbol(struct MachoHandle* macho_handle);
void print_symbols(struct MachoHandle* macho_handle);
void print_section_type(struct section_64* sect);
void print_segments_and_sections(struct MachoHandle* macho_handle);
void print_file_type(struct MachoHandle* handle);
void print_command_type(uint32_t cmd);


#endif

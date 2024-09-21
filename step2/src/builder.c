#include "../include/builder.h"
#include <mach-o/loader.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Builder* new_builder(void) {
    return calloc(1, sizeof(struct Builder));
}

void builder_set_input_text_segment_handle(struct Builder* builder, struct SegmentHandle* input_text_segment_handle) {
    builder->input_text_segment_handle = input_text_segment_handle;
}

bool build_segment_text(struct Builder* builder) {
    builder->output_text_segment_handle = malloc(sizeof(struct SegmentHandle));
    builder->output_text_segment_handle->load_cmd = (struct segment_command_64){   
                                                                   .cmd = LC_SEGMENT_64,
                                                                   .cmdsize = builder->input_text_segment_handle->load_cmd.cmdsize,
                                                                   .fileoff =  0,
                                                                   .filesize = PAGE_SIZE,
                                                                   .vmaddr = 0x0000000100000000,
                                                                   .vmsize = 0x0000000000004000, // (page size)
                                                                   .maxprot = VM_PROT_READ | VM_PROT_EXECUTE,
                                                                   .initprot = VM_PROT_READ | VM_PROT_EXECUTE,
                                                                   .nsects = builder->input_text_segment_handle->load_cmd.nsects,
                                                                   .flags = 0
    };
    builder->output_text_segment_handle->segment_name = malloc(sizeof(uint8_t) * strlen(SEG_TEXT));
    strcpy(builder->output_text_segment_handle->segment_name, SEG_TEXT);
    strcpy(builder->output_text_segment_handle->load_cmd.segname, SEG_TEXT);

    // Now build the sections corresponding to this segment:
    builder->output_text_segment_handle->sections = new_vector_of_sections();
    struct SectionHandle* input_text_section = get_section_handle_with_section_name(builder->input_text_segment_handle->sections, SECT_TEXT); 
    if (!input_text_section) {
        fprintf(stderr, "Builder: Input text section not found :(\n");
        return false;
    }

    struct SectionHandle* output_text_section = malloc(sizeof(struct SectionHandle));
    output_text_section->section_cmd = (struct section_64){ .addr = 0x0000000100000000 + 0x3f90,
                                                            .size = input_text_section->section_cmd.size,
                                                            .offset = 16272,
                                                            .align = 2,
                                                            .reloff = 0,
                                                            .nreloc = 0,
                                                            .flags = S_ATTR_PURE_INSTRUCTIONS | S_ATTR_SOME_INSTRUCTIONS | S_REGULAR,
                                                            .reserved1 = 0,
                                                            .reserved2 = 0,
                                                            .reserved3 = 0
    };
    output_text_section->section_name = malloc(sizeof(uint8_t) * strlen(SECT_TEXT));
    strcpy(output_text_section->section_name, SECT_TEXT);
    strcpy(output_text_section->section_cmd.sectname, SECT_TEXT);
    strcpy(output_text_section->section_cmd.segname, SEG_TEXT);


    // For now only copy the contents of this unique input file, of course a real linker here would copy the contents
    // of all text section of each input file
    if (!input_text_section->raw_data_of_section) {
        fprintf(stderr, "Builder: Input text section has no contents to copy\n");
        return false;
    }
    output_text_section->raw_data_of_section = calloc(input_text_section->section_cmd.size, sizeof(uint8_t));
    memcpy(output_text_section->raw_data_of_section, input_text_section->raw_data_of_section, input_text_section->section_cmd.size);

    push_section(builder->output_text_segment_handle->sections, output_text_section);

    // __compact_unwind in the final executable it appears to be in another segment, so we'll just put the text section here.

    return true;
}



#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <sys/_types/_off_t.h>
#include <unistd.h>
#include <errno.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <sys/_types/_size_t.h>
#include <mach-o/fixup-chains.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <CommonCrypto/CommonDigest.h>
#include <sys/mman.h>
#include "../../not_really_cs_blobs.h"
#include "../include/helpers.h"
#include "../include/macho_parser.h"
#include "../include/debug.h"
#include "../include/writer.h"


void write_macho_header_2(struct OutputFile* output_file, size_t* file_offset) {
    /*
    struct mach_header_64 {
		uint32_t	magic;			// mach magic number identifier
		cpu_type_t	cputype;		// cpu specifier
		cpu_subtype_t	cpusubtype;	// machine specifier
		uint32_t	filetype;		// type of file
		uint32_t	ncmds;			// number of load commands
		uint32_t	sizeofcmds;		// the size of all the load commands
		uint32_t	flags;			// flags
		uint32_t	reserved;		// reserved; 64 bit only
	};
	*/
	uint8_t* buf = output_file->buffer + *file_offset;
	memset(buf, 0, sizeof(struct mach_header_64));
	
	struct mach_header_64* header = (struct mach_header_64 *)buf;
	
	header->magic = MH_MAGIC_64;
	header->cputype = CPU_TYPE_ARM64;
	header->cpusubtype = 0;
	header->filetype = MH_EXECUTE;
	header->ncmds = 0;                                      // THIS
	header->sizeofcmds = 0;                                 // AND THIS should be set by a parameter
	header->flags = MH_NOUNDEFS | MH_DYLDLINK | MH_TWOLEVEL | MH_PIE;
	
	*file_offset += sizeof(struct mach_header_64);
}


void write_segment_page_zero(struct OutputFile* output_file, size_t* file_offset)
{
	uint8_t* buf = output_file->buffer + *file_offset;
	memset(buf, 0, sizeof(struct segment_command_64));
	
	struct segment_command_64* load_command_zero = (struct segment_command_64 *)buf;
	
	load_command_zero->cmd = LC_SEGMENT_64;
	load_command_zero->cmdsize = 72;					// includes sizeof section_64 structs 
	strcpy(load_command_zero->segname,SEG_PAGEZERO);	// segment name 
	load_command_zero->vmaddr	= 0x0000000000000000;	// memory address of this segment 
	load_command_zero->vmsize	= 0x0000000100000000;	// memory size of this segment 
	load_command_zero->fileoff = 0;						// file offset of this segment 
	load_command_zero->filesize = 0;					// amount to map from the file 
	load_command_zero->maxprot = 0;						// maximum VM protection 
	load_command_zero->initprot = 0; 					// initial VM protection 
	load_command_zero->nsects	= 0;					// number of sections in segment 
	load_command_zero->flags = 0;						// flags 
	
	*file_offset += sizeof(struct segment_command_64);
	output_file->total_size_for_load_cmds += load_command_zero->cmdsize;
}

void write_segment_text(struct OutputFile* output_file,
							size_t* file_offset,
							struct SegmentHandle* segment_handle)
{
	uint8_t* buf = output_file->buffer + *file_offset;
	
    memcpy(buf, &(segment_handle->load_cmd), sizeof(struct segment_command_64));
	
	*file_offset += sizeof(struct segment_command_64);
	output_file->total_size_for_load_cmds += segment_handle->load_cmd.cmdsize;
}

bool write_section_text_2(struct OutputFile* output_file,
                            size_t* file_offset,
                            struct SegmentHandle* segment_handle)
{
	uint8_t* buf = output_file->buffer + *file_offset;

    struct SectionHandle* text_sect_handle = get_section_handle_with_section_name(segment_handle->sections, SECT_TEXT);
    if (!text_sect_handle) {
        return false;
    }

    memcpy(buf, &(text_sect_handle->section_cmd), sizeof(struct section_64));

    *file_offset += sizeof(struct section_64);
    return true;
}

bool write_contents_of_section_text(struct OutputFile *output_file,
                                    struct SegmentHandle *segment_handle) {

    struct SectionHandle* text_sect_handle = get_section_handle_with_section_name(segment_handle->sections, SECT_TEXT);
    if (!text_sect_handle) {
        return false;
    }

    uint8_t* buf = output_file->buffer + text_sect_handle->section_cmd.offset;
    uint8_t* text_contents = text_sect_handle->raw_data_of_section;
    if (!text_contents) {
        return false;
    }
    size_t size_of_text_contents = text_sect_handle->section_cmd.size;

    memcpy(buf, text_contents, size_of_text_contents);

    return true;
}

void dump_output_buffer_into_file(struct OutputFile* output_file)
{
	size_t written = fwrite(output_file->buffer, output_file->filesize, 1, output_file->fptr);
	if (written < 1) {
		fprintf(stderr, "Error while writing the output buffer into the file.\n");
		exit(1);
	}
}

struct OutputFile* writer_generate_executable_file(char* filename, struct Builder* builder)
{
	// Create the file itself:
	char* filename_full_path = malloc(strlen("../bin/") + strlen(filename));
    strcat(filename_full_path, "../bin/");
    strcat(filename_full_path, filename);

	struct OutputFile* output_file = malloc(sizeof(struct OutputFile));
	output_file->filename = malloc(strlen(filename));
	strcpy(output_file->filename, filename);

    FILE* fptr_out = create_file(filename_full_path);
	if (!fptr_out) {
		fprintf(stderr, "Writer: Error while creating file.\n");
		exit(1);
	}
    output_file->filesize = PAGE_SIZE; //  TODO: this is wrong of course. We should do a first pass to calculate the filesize
                                       // of the resulting binary. For now let's hardcode it
	output_file->fptr = fptr_out;
	output_file->buffer = calloc(output_file->filesize, sizeof(uint8_t));
	output_file->total_size_for_load_cmds = 0;

    size_t file_offset = 0;

	write_macho_header_2(output_file, &file_offset);

    write_segment_page_zero(output_file, &file_offset);

    // The Builder is in the middle, between the Parser and the Writer.
    // So, to write the output text segment, the Writer asks for it to the Builder.
    // But in turn, the Builder built it by using the info given to it by the Parser.
    // BUT the Writer should not talk directly to the Parser.
    // So, for now the main function will be the one orchestrating everything.

    write_segment_text(output_file, &file_offset, builder->output_text_segment_handle);
    
    if (!write_section_text_2(output_file, &file_offset, builder->output_text_segment_handle)) {
        fprintf(stderr, "Writer: Error while writing the text section.\n");
        exit(1);
    }

    if (!write_contents_of_section_text(output_file, builder->output_text_segment_handle)) {
        fprintf(stderr, "Writer: Error while writing the contents of the text section.\n");
        exit(1);
    }


    // Finally write the file itself
	dump_output_buffer_into_file(output_file);

    #if DEBUG
	printf("final value of file_ofsset: %zu\n", file_offset);
	#endif



    return output_file;
}
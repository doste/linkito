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
#include "../include/builder.h"


/*
This step is actually the fourth:
4. Once the blobs are eliminated, improve the code so that it can take a single
   input object file with the empty `main` to produce the same output.

So now, in this step what we are doing is generating the same file we receive as input, 
*but* of course doing it 'dynamically' not hardcoding it like we did in step1.

The design I'm thinking is something like this:

    [macho_parser]  	----------->    [ builder ]	----------->   [writer]

	Takes input files,				Builds segments and				Finally write the output file (an executable)
	build the corresponding         sections
	structures for each
	(one MachoHandle for
	each mach-o input file).



*/


// 			********** Header ********** 

void write_macho_header(struct OutputFile* output_file, size_t* file_offset)
{
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
	header->ncmds = 16;
	header->sizeofcmds = 0; //744;
	header->flags = MH_NOUNDEFS | MH_DYLDLINK | MH_TWOLEVEL | MH_PIE;
	
	*file_offset += sizeof(struct mach_header_64);
}

void update_macho_header_total_size_for_load_cmds(struct OutputFile* output_file) {
	uint8_t* buf = output_file->buffer + 0;
	struct mach_header_64* header = (struct mach_header_64 *)buf;
	header->sizeofcmds = output_file->total_size_for_load_cmds;  //744;
}

void dump_whole_buffer_into_file(struct OutputFile* output_file)
{
	size_t written = fwrite(output_file->buffer, output_file->filesize, 1, output_file->fptr);
	if (written < 1) {
		fprintf(stderr, "Error while writing the buffer into the file.\n");
		exit(1);
	}
}


// 			********** Load commands ********** 

/*
 * The load commands directly follow the mach_header.  The total size of all
 * of the commands is given by the sizeofcmds field in the mach_header.  All
 * load commands must have as their first two fields cmd and cmdsize.  The cmd
 * field is filled in with a constant for that command type.  Each command type
 * has a structure specifically for it.  The cmdsize field is the size in bytes
 * of the particular load command structure plus anything that follows it that
 * is a part of the load command (i.e. section structures, strings, etc.).  To
 * advance to the next load command the cmdsize can be added to the offset or
 * pointer of the current load command.  The cmdsize for 32-bit architectures
 * MUST be a multiple of 4 bytes and for 64-bit architectures MUST be a multiple
 * of 8 bytes (these are forever the maximum alignment of any load commands).
 * sizeof(long) (this is forever the maximum alignment of any load commands).
 * The padded bytes must be zero.  All tables in the object file must also
 * follow these rules so the file can be memory mapped.  Otherwise the pointers
 * to these tables will not work well or at all on some machines.  With all
 * padding zeroed like objects will compare byte for byte.
 
struct load_command {
	unsigned long cmd;			// type of load command 
	unsigned long cmdsize;		// total size of command in bytes 
};
*/


/*		Step2: This should remain the same.
Load command 0
	  cmd LC_SEGMENT_64
  cmdsize 72
  segname __PAGEZERO
   vmaddr 0x0000000000000000
   vmsize 0x0000000100000000
  fileoff 0
 filesize 0
  maxprot 0x00000000
 initprot 0x00000000
   nsects 0
	flags 0x0
*/
void write_load_command_zero(struct OutputFile* output_file, size_t* file_offset)
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

/*
Load command 1
	  cmd LC_SEGMENT_64
  cmdsize 232
  segname __TEXT
   vmaddr 0x0000000100000000
   vmsize 0x0000000000004000
  fileoff 0
 filesize 16384
  maxprot 0x00000005
 initprot 0x00000005
   nsects 2
	flags 0x0
*/
void write_load_command_one(struct OutputFile* output_file,
							size_t* file_offset,
							size_t* offset_of_exec_segment,
							size_t*size_of_exec_segment)
{
	uint8_t* buf = output_file->buffer + *file_offset;
	memset(buf, 0, sizeof(struct segment_command_64));
	
	struct segment_command_64* load_command_one = (struct segment_command_64 *)buf;
	
	load_command_one->cmd = LC_SEGMENT_64;
	load_command_one->cmdsize = 232;									// includes sizeof section_64 structs 
	strcpy(load_command_one->segname, SEG_TEXT);						// segment name 
	load_command_one->vmaddr	= 0x0000000100000000;					// memory address of this segment 
	load_command_one->vmsize	= 0x0000000000004000;					// memory size of this segment 
	load_command_one->fileoff = 0;										// file offset of this segment 
	load_command_one->filesize = 16384;									// amount to map from the file 
	load_command_one->maxprot = VM_PROT_READ | VM_PROT_EXECUTE;			// maximum VM protection 
	load_command_one->initprot =  VM_PROT_READ | VM_PROT_EXECUTE; 		// initial VM protection 
	load_command_one->nsects	= 2;									// number of sections in segment 
	load_command_one->flags = 0;										// flags 
	
	*file_offset += sizeof(struct segment_command_64);
	*offset_of_exec_segment = load_command_one->fileoff;
	*size_of_exec_segment = load_command_one->filesize;
	output_file->total_size_for_load_cmds += load_command_one->cmdsize;
}


/*
Section
  sectname __text
   segname __TEXT
	  addr 0x0000000100003f90
	  size 0x0000000000000018
	offset 16272
	 align 2^2 (4)
	reloff 0
	nreloc 0
	 flags 0x80000400
 reserved1 0
 reserved2 0
*/
void write_section_text(struct OutputFile* output_file, size_t* file_offset, size_t* file_offset_for_text_contents)
{
	uint8_t* buf = output_file->buffer + *file_offset;
	memset(buf, 0, sizeof(struct section_64));
	
	struct section_64* text_section = (struct section_64 *)buf;
	
	strcpy(text_section->sectname, SECT_TEXT);
	strcpy(text_section->segname, SEG_TEXT);
	text_section->addr = 0x0000000100003f90;
	text_section->size = 0x0000000000000018;
	text_section->offset = 16272;
	text_section->align  = 2;
	text_section->reloff = 0;
	text_section->nreloc = 0;
	text_section->flags = S_ATTR_PURE_INSTRUCTIONS | S_ATTR_SOME_INSTRUCTIONS | S_REGULAR;
	text_section->reserved1 = 0;
	text_section->reserved2 = 0;
	
	*file_offset += sizeof(struct section_64);
	*file_offset_for_text_contents = text_section->offset;
}

/*
Section
  sectname __unwind_info
   segname __TEXT
	  addr 0x0000000100003fa8
	  size 0x0000000000000058
	offset 16296
	 align 2^2 (4)
	reloff 0
	nreloc 0
	 flags 0x00000000
 reserved1 0
 reserved2 0
*/
void write_section_unwind_info(struct OutputFile* output_file, size_t* file_offset, size_t* file_offset_unwind_info)
{
	uint8_t* buf = output_file->buffer + *file_offset;
	memset(buf, 0, sizeof(struct section_64));
	
	struct section_64* unwind_info_section = (struct section_64 *)buf;
	
	strcpy(unwind_info_section->sectname, "__unwind_info");
	strcpy(unwind_info_section->segname, SEG_TEXT);
	unwind_info_section->addr = 0x0000000100003fa8;
	unwind_info_section->size = 0x0000000000000058;
	unwind_info_section->offset = 16296;
	unwind_info_section->align  = 2;
	unwind_info_section->reloff = 0;
	unwind_info_section->nreloc = 0;
	unwind_info_section->flags = 0;
	unwind_info_section->reserved1 = 0;
	unwind_info_section->reserved2 = 0;
	
	*file_offset += sizeof(struct section_64);
	*file_offset_unwind_info = unwind_info_section->offset;
}


/*
Load command 2
	  cmd LC_SEGMENT_64
  cmdsize 72
  segname __LINKEDIT
   vmaddr 0x0000000100004000
   vmsize 0x0000000000004000
  fileoff 16384
 filesize 464
  maxprot 0x00000001
 initprot 0x00000001
   nsects 0
	flags 0x0
*/
void write_load_command_two(struct OutputFile* output_file, size_t* file_offset)
{
	uint8_t* buf = output_file->buffer + *file_offset;
	memset(buf, 0, sizeof(struct segment_command_64));
	
	struct segment_command_64* load_command = (struct segment_command_64 *)buf;
	
	load_command->cmd = LC_SEGMENT_64;
	load_command->cmdsize = 72;									// includes sizeof section_64 structs 
	strcpy(load_command->segname, SEG_LINKEDIT);				// segment name 
	load_command->vmaddr	= 0x0000000100004000;				// memory address of this segment 
	load_command->vmsize	= 0x0000000000004000;				// memory size of this segment 
	load_command->fileoff = 16384;								// file offset of this segment 
	load_command->filesize = 480 /*original: 464 . 384*/;	    // amount to map from the file 
	load_command->maxprot = VM_PROT_READ;						// maximum VM protection 
	load_command->initprot =  VM_PROT_READ; 					// initial VM protection 
	load_command->nsects	= 0;								// number of sections in segment 
	load_command->flags = 0;									// flags 
	
	*file_offset += sizeof(struct segment_command_64);
	output_file->total_size_for_load_cmds += load_command->cmdsize;
	
	// __LINKEDIT fileoff+filesize should == LC_CODE_SIGNATURE dataoff+datasize
}

/*
Load command 3
	  cmd LC_DYLD_CHAINED_FIXUPS
  cmdsize 16
  dataoff 16384
 datasize 56
*/
void write_load_command_three(struct OutputFile* output_file, size_t* file_offset, size_t* offset_of_dyld_chained_fixups_in_linkedit_segment)
{
	uint8_t* buf = output_file->buffer + *file_offset;
	memset(buf, 0, sizeof(struct linkedit_data_command));
	
	struct linkedit_data_command* load_command = (struct linkedit_data_command *)buf;
	
	load_command->cmd = LC_DYLD_CHAINED_FIXUPS;
	load_command->cmdsize = sizeof(struct linkedit_data_command);		// includes sizeof section_64 structs 
	load_command->dataoff = 16384;										// file offset of data in __LINKEDIT segment 
	load_command->datasize = 56;										// file size of data in __LINKEDIT segment
	
	*file_offset += sizeof(struct linkedit_data_command);
	*offset_of_dyld_chained_fixups_in_linkedit_segment = load_command->dataoff;
	output_file->total_size_for_load_cmds += load_command->cmdsize;
}

/*
Load command 4
	  cmd LC_DYLD_EXPORTS_TRIE
  cmdsize 16
  dataoff 16440
 datasize 48
*/
void write_load_command_four(struct OutputFile* output_file, size_t* file_offset, size_t* offset_of_dyld_exports_trie_in_linkedit_segment)
{
	uint8_t* buf = output_file->buffer + *file_offset;
	memset(buf, 0, sizeof(struct linkedit_data_command));
	
	struct linkedit_data_command* load_command = (struct linkedit_data_command *)buf;
	
	load_command->cmd = LC_DYLD_EXPORTS_TRIE;
	load_command->cmdsize = sizeof(struct linkedit_data_command);		// includes sizeof section_64 structs 
	load_command->dataoff = 16440;										// file offset of data in __LINKEDIT segment 
	load_command->datasize = 48;										// file size of data in __LINKEDIT segment
	
	*file_offset += sizeof(struct linkedit_data_command);
	*offset_of_dyld_exports_trie_in_linkedit_segment = load_command->dataoff;
	output_file->total_size_for_load_cmds += load_command->cmdsize;
}

/*
Load command 5
	 cmd LC_SYMTAB
 cmdsize 24
  symoff 16496
   nsyms 2
  stroff 16528
 strsize 32
*/
void write_load_command_five(struct OutputFile* output_file,
							size_t* file_offset,
							size_t* file_offset_for_string_table,
							size_t* size_of_string_table,
							size_t* file_offset_for_symbol_table,
							size_t* number_of_symbols)
{
	uint8_t* buf = output_file->buffer + *file_offset;
	memset(buf, 0, sizeof(struct symtab_command));
	
	struct symtab_command* load_command = (struct symtab_command *)buf;
	
	load_command->cmd = LC_SYMTAB;
	load_command->cmdsize = sizeof(struct symtab_command);		
	load_command->symoff = 16496;										// symbol table offset 
	load_command->nsyms = 2;											// number of symbol table entries 
	load_command->stroff = 16528;										// string table offset 
	load_command->strsize = 32;											// string table size in bytes 
	
	*file_offset += sizeof(struct symtab_command);
	*file_offset_for_string_table = load_command->stroff;
	*size_of_string_table = load_command->strsize;
	*file_offset_for_symbol_table = load_command->symoff;
	*number_of_symbols = load_command->nsyms;
	output_file->total_size_for_load_cmds += load_command->cmdsize;
}

/*
Load command 6
			cmd LC_DYSYMTAB
		cmdsize 80
	  ilocalsym 0
	  nlocalsym 0
	 iextdefsym 0
	 nextdefsym 2
	  iundefsym 2
	  nundefsym 0
		 tocoff 0
		   ntoc 0
	  modtaboff 0
		nmodtab 0
   extrefsymoff 0
	nextrefsyms 0
 indirectsymoff 0
  nindirectsyms 0
	  extreloff 0
		nextrel 0
	  locreloff 0
		nlocrel 0
*/
void write_load_command_six(struct OutputFile* output_file, size_t* file_offset)
{
	uint8_t* buf = output_file->buffer + *file_offset;
	memset(buf, 0, sizeof(struct dysymtab_command));
	
	struct dysymtab_command* load_command = (struct dysymtab_command *)buf;
	
	load_command->cmd = LC_DYSYMTAB;
	load_command->cmdsize = sizeof(struct dysymtab_command);
	load_command->ilocalsym = 0;
	load_command->nlocalsym  = 0;
	load_command->iextdefsym  = 0;
	load_command->nextdefsym  = 2;
	load_command->iundefsym  = 2;
	load_command->nundefsym  = 0;
	load_command->tocoff  = 0;
	load_command->ntoc  = 0;
	load_command->modtaboff  = 0;
	load_command->nmodtab  = 0;
	load_command->extrefsymoff  = 0;
	load_command->nextrefsyms  = 0;
	load_command->indirectsymoff  = 0;
	load_command->nindirectsyms  = 0;
	load_command->extreloff  = 0;
	load_command->nextrel  = 0;
	load_command->locreloff  = 0;
	load_command->nlocrel  = 0;
	
	*file_offset += sizeof(struct dysymtab_command);
	
	output_file->total_size_for_load_cmds += load_command->cmdsize;
}

/*
Load command 7
		  cmd LC_LOAD_DYLINKER
	  cmdsize 32
		 name /usr/lib/dyld (offset 12)
*/
void write_load_command_seven(struct OutputFile* output_file, size_t* file_offset)
{
	uint8_t* buf = output_file->buffer + *file_offset;
	memset(buf, 0, sizeof(struct dylinker_command));
	
	struct dylinker_command* load_command = (struct dylinker_command *)buf;
	
	load_command->cmd = LC_LOAD_DYLINKER;
	//load_command->cmdsize = sizeof(struct dylinker_command) + strlen("/usr/lib/dyld") + 7; // 7 so cmdsize is multiple of 8   // includes pathname string
	char* name = "/usr/lib/dyld";
	size_t name_size_aligned = align_to(strlen(name) + 1, 16);
	load_command->cmdsize = sizeof(struct dylinker_command) + name_size_aligned;
	load_command->name.offset = 12;
	
	*file_offset += sizeof(struct dylinker_command); // So we start writing the pathname string at the correct offset 
	output_file->total_size_for_load_cmds += load_command->cmdsize;
}

/*
Load command 8
	 cmd LC_UUID
 cmdsize 24
	uuid 7781ED18-5267-39C5-8343-559A3CE11B1C
	
 * The uuid load command contains a single 128-bit unique random number that
 * identifies an object produced by the static link editor.
*/
void write_load_command_eight(struct OutputFile* output_file, size_t* file_offset)
{
	uint8_t* buf = output_file->buffer + *file_offset;
	memset(buf, 0, sizeof(struct uuid_command));
	
	struct uuid_command* load_command = (struct uuid_command *)buf;
	
	load_command->cmd = LC_UUID;
	load_command->cmdsize = sizeof(struct uuid_command);
	uint8_t uuid[16] = {0x78, 0x81, 0xEE, 0x18, 0x51, 0x67, 0x40, 0xC5, 0x83, 0x43, 0x55, 0x9B, 0x3C, 0xE0, 0x1B, 0x1C};
	uuid[6] = (uuid[6] & 0b00001111) | 0b01010000;
	uuid[8] = (uuid[8] & 0b00111111) | 0b10000000;
	memcpy(load_command->uuid, uuid, 16);
	
	*file_offset += sizeof(struct uuid_command); 
	output_file->total_size_for_load_cmds += load_command->cmdsize;
}

/*
Load command 9
	  cmd LC_BUILD_VERSION
  cmdsize 32
 platform 1
	minos 14.0
	  sdk 14.5
   ntools 1
	 tool 3
  version 1053.12
*/
void write_load_command_nine(struct OutputFile* output_file, size_t* file_offset)
{
	uint8_t* buf = output_file->buffer + *file_offset;
	memset(buf, 0, sizeof(struct build_version_command));
	
	struct build_version_command* load_command = (struct build_version_command *)buf;
	
	load_command->cmd = LC_BUILD_VERSION;
	load_command->cmdsize = sizeof(struct build_version_command) + 1 * sizeof(struct build_tool_version);		// sizeof(struct build_version_command) plus ntools * sizeof(struct build_tool_version)
	load_command->platform = PLATFORM_MACOS;					// platform 
	load_command->minos = (14 << 16);							// X.Y.Z is encoded in nibbles xxxx.yy.zz         	===>    X = 14 Y = 0  ===>  minos = (14 << 16)
	load_command->sdk = (14 << 16) | (5 << 8);					// X.Y.Z is encoded in nibbles xxxx.yy.zz 			===>    X = 14 Y = 5  ===>  sdk = (14 << 16) | (5 << 8)
	load_command->ntools = 1;									// number of tool entries following this 
	
	*file_offset += sizeof(struct build_version_command); // So, after it we can start writing the build_tool_version(s)
	output_file->total_size_for_load_cmds += load_command->cmdsize;
}

/*
Load command 10
	  cmd LC_SOURCE_VERSION
  cmdsize 16
  version 0.0
*/
void write_load_command_ten(struct OutputFile* output_file, size_t* file_offset)
{
	uint8_t* buf = output_file->buffer + *file_offset;
	memset(buf, 0, sizeof(struct source_version_command));
	struct source_version_command* load_command = (struct source_version_command *)buf;
	
	load_command->cmd = LC_SOURCE_VERSION;
	load_command->cmdsize = 16;		
	load_command->version = 0;					// A.B.C.D.E packed as a24.b10.c10.d10.e10
	
	*file_offset += sizeof(struct source_version_command);
	output_file->total_size_for_load_cmds += load_command->cmdsize;
}

/*
Load command 11
	   cmd LC_MAIN
   cmdsize 24
  entryoff 16272
 stacksize 0
*/
void write_load_command_eleven(struct OutputFile* output_file, size_t* file_offset)
{
	uint8_t* buf = output_file->buffer + *file_offset;
	memset(buf, 0, sizeof(struct entry_point_command));
	struct entry_point_command* load_command = (struct entry_point_command *)buf;
	
	load_command->cmd = LC_MAIN;
	load_command->cmdsize = 24;	
	load_command->entryoff = 16272;							// file (__TEXT) offset of main() 
	load_command->stacksize	= 0;							// if not zero, initial stack size 
	
	*file_offset += sizeof(struct entry_point_command);
	output_file->total_size_for_load_cmds += load_command->cmdsize;
}

/*
Load command 12
		  cmd LC_LOAD_DYLIB
	  cmdsize 56
		 name /usr/lib/libSystem.B.dylib (offset 24)
   time stamp 2 Wed Dec 31 21:00:02 1969
	  current version 1345.120.2
compatibility version 1.0.0 
*/
void write_load_command_twelve(struct OutputFile* output_file, size_t* file_offset)
{
	uint8_t* buf = output_file->buffer + *file_offset;
	memset(buf, 0, sizeof(struct dylib_command));
	struct dylib_command* load_command = (struct dylib_command *)buf;
	
	char* name = "/usr/lib/libSystem.B.dylib";
	size_t name_size_aligned = align_to(strlen(name) + 1, 16);
	
	load_command->cmd = LC_LOAD_DYLIB;
	load_command->cmdsize = sizeof(struct dylib_command) + name_size_aligned;
	load_command->dylib = (struct dylib) {.name.offset = sizeof(struct dylib_command),   			// the string corresponding to the name would be just after this struct, so the offset would be the sum of the sizes of the following fields. The offset is from the start of the load command structure! So that sum should include the whole struct (this struct dylib + struct dylib_command)
							  .timestamp = 2,   		    // from https://www.unixtimestamp.com/ IDK
							  .current_version = (1345 << 16 | 120 << 8 | 2),
							  .compatibility_version = (1 << 16)
	 };	// the library identification
	
	*file_offset += sizeof(struct dylib_command);
	output_file->total_size_for_load_cmds += load_command->cmdsize;
}

/*
Load command 13
	  cmd LC_FUNCTION_STARTS
  cmdsize 16
  dataoff 16488
 datasize 8
*/
void write_load_command_thirteen(struct OutputFile* output_file, size_t* file_offset, size_t* offset_of_function_starts)
{
	uint8_t* buf = output_file->buffer + *file_offset;
	memset(buf, 0, sizeof(struct linkedit_data_command));
	struct linkedit_data_command* load_command = (struct linkedit_data_command *)buf;
	
	load_command->cmd = LC_FUNCTION_STARTS;
	load_command->cmdsize = sizeof(struct linkedit_data_command);		
	load_command->dataoff = 16488;
	load_command->datasize = 8;
	
	*file_offset += sizeof(struct linkedit_data_command);
	*offset_of_function_starts = load_command->dataoff;
	output_file->total_size_for_load_cmds += load_command->cmdsize;
}

/*
Load command 14
	  cmd LC_DATA_IN_CODE
  cmdsize 16
  dataoff 16496
 datasize 0
*/
void write_load_command_fourteen(struct OutputFile* output_file, size_t* file_offset)
{
	uint8_t* buf = output_file->buffer + *file_offset;
	memset(buf, 0, sizeof(struct linkedit_data_command));
	struct linkedit_data_command* load_command = (struct linkedit_data_command *)buf;
	
	load_command->cmd = LC_DATA_IN_CODE;
	load_command->cmdsize = sizeof(struct linkedit_data_command);		
	load_command->dataoff = 16496;
	load_command->datasize = 0;
	
	*file_offset += sizeof(struct linkedit_data_command);
	output_file->total_size_for_load_cmds += load_command->cmdsize;
}

/*
Load command 15
	  cmd LC_CODE_SIGNATURE
  cmdsize 16
  dataoff 16560
 datasize 288
*/
void write_load_command_fifteen(struct OutputFile* output_file, size_t* file_offset, size_t offset_for_code_sign, size_t size_of_code_sign)
{
	uint8_t* buf = output_file->buffer + *file_offset;
	memset(buf, 0, sizeof(struct linkedit_data_command));
	struct linkedit_data_command* load_command = (struct linkedit_data_command *)buf;
	
	load_command->cmd = LC_CODE_SIGNATURE;
	load_command->cmdsize = sizeof(struct linkedit_data_command);		
	load_command->dataoff = offset_for_code_sign; 						//dataoffset
	load_command->datasize = size_of_code_sign;   						//datasize
	
	*file_offset += sizeof(struct linkedit_data_command);
	output_file->total_size_for_load_cmds += load_command->cmdsize;
}




void write_string_table(struct OutputFile* output_file, size_t file_offset_for_string_table, size_t size_of_string_table)
{
	uint8_t* buf = output_file->buffer + file_offset_for_string_table;
	memset(buf, 0, size_of_string_table);
	
	*buf = (char)2;		// ascii value for 'start of text' ::shrug::
	memcpy(buf + 2, "__mh_execute_header", strlen("__mh_execute_header"));
	memcpy(buf + 2 + strlen("__mh_execute_header") + 1, "_main", strlen("_main"));
}

void write_symbol_table(struct OutputFile* output_file, size_t file_offset_for_symbol_table, size_t number_of_symbols)
{
	uint8_t* buf = output_file->buffer + file_offset_for_symbol_table;
	size_t size_of_string_table = number_of_symbols * sizeof(struct nlist_64);
	memset(buf, 0, size_of_string_table);
	
	/*
	struct nlist_64 {
		union {
			uint32_t n_strx;  // index into the string table 
		} n_un;
		uint8_t n_type;       // type flag, see below 
		uint8_t n_sect;       // section number or NO_SECT 
		uint16_t n_desc;      // see <mach-o/stab.h> 
		uint64_t n_value;     // value of this symbol (or stab offset) 
	};
	*/
	
	struct nlist_64* a_symbol = (struct nlist_64 *)buf;
	
	a_symbol->n_un.n_strx = 2;
	a_symbol->n_type = 15;
	a_symbol->n_sect = 1;
	a_symbol->n_desc = 0x10;
	a_symbol->n_value = 4294967296;
	
	buf += sizeof(struct nlist_64);
	
	a_symbol = (struct nlist_64 *)buf;
	a_symbol->n_un.n_strx = 22;
	a_symbol->n_type = 15;
	a_symbol->n_sect = 1;
	a_symbol->n_desc = 0x00;
	a_symbol->n_value = 4294983568;
	
	//*file_offset += number_of_symbols * sizeof(struct nlist_64); 
}

///// straight copy of sold/mold (https://github.com/bluewhalesystems/sold/blob/main/macho/output-chunks.cc#L1386)

uint32_t system_page_size = 16384;
//uint32_t system_page_size = 0x1000;

#define SHA256_SIZE 32


//template <typename E>
//void CodeSignatureSection<E>::compute_size(Context<E> &ctx) {
size_t compute_size_for_code_signature_section(char* filename, uint32_t offset_of_code_signature_data) {
  //std::string filename = filepath(ctx.arg.final_output).filename().string();
  //int64_t filename_size = align_to(filename.size() + 1, 16);
  //int64_t num_blocks = align_to(this->hdr.offset, E::page_size) / E::page_size;
  //this->hdr.size = sizeof(CodeSignatureHeader) + sizeof(CodeSignatureBlobIndex) +
	//			   sizeof(CodeSignatureDirectory) + filename_size +
	//			   num_blocks * SHA256_SIZE;
				   
	int64_t filename_size = align_to(strlen(filename) + 1, 16);
    int64_t num_blocks = align_to(offset_of_code_signature_data, system_page_size) / system_page_size;
						// With this, we are first aligning the offset of the code sign section to a page size,
						// then dividing that number by the page size, this way we divide all the file up to this offset
						// in chunks of page size
			   
  return sizeof(struct CodeSignatureHeader) + sizeof(struct CodeSignatureBlobIndex) +
	 sizeof(struct CodeSignatureDirectory) + filename_size +
	 num_blocks * SHA256_SIZE;
}




//template <typename E>
//void CodeSignatureSection<E>::write_signature(Context<E> &ctx) {
  //Timer t(ctx, "write_signature");
void write_signature(struct OutputFile* output_file,
					  size_t offset_of_code_signature_data, 
					  size_t size_of_code_signature_data,
				  	  size_t offset_of_exec_segment,
					  size_t size_of_exec_segment) {


  //u8 *buf = ctx.buf + this->hdr.offset;
  //memset(buf, 0, this->hdr.size);
  uint8_t* buf = output_file->buffer + offset_of_code_signature_data;
  memset(buf, 0, size_of_code_signature_data);

  //std::string filename = filepath(ctx.arg.final_output).filename().string();
  //i64 filename_size = align_to(filename.size() + 1, 16);
  //i64 num_blocks = align_to(this->hdr.offset, E::page_size) / E::page_size;

  int64_t filename_size = align_to(strlen(output_file->filename) + 1, 16);
  int64_t num_blocks = align_to(offset_of_code_signature_data, system_page_size) / system_page_size;
  //printf("num_blocks := %lld\n", num_blocks);

  // Fill code-sign header fields
  //CodeSignatureHeader &sighdr = *(CodeSignatureHeader *)buf;
  struct CodeSignatureHeader* sighdr = (struct CodeSignatureHeader *)buf;
  buf += sizeof(*sighdr);

  sighdr->magic = le_to_be_32((uint32_t) CSMAGIC_EMBEDDED_SIGNATURE);
  //printf("size_of_code_signature_data := 0x%02X\n", (unsigned int)(size_of_code_signature_data));
  sighdr->length = le_to_be_32((uint32_t) size_of_code_signature_data);
  sighdr->count = le_to_be_32((uint32_t) 1);

  //CodeSignatureBlobIndex &idx = *(CodeSignatureBlobIndex *)buf;
  struct CodeSignatureBlobIndex* idx = (struct CodeSignatureBlobIndex *)buf;
  buf += sizeof(*idx);

  idx->type = le_to_be_32((uint32_t) CSSLOT_CODEDIRECTORY);
  idx->offset = le_to_be_32((uint32_t) sizeof(*sighdr) + sizeof(*idx));
  //printf("este offset es := 0x%02X\n", (unsigned int)(idx->offset));

  //CodeSignatureDirectory &dir = *(CodeSignatureDirectory *)buf;
  struct CodeSignatureDirectory* dir = (struct CodeSignatureDirectory *)buf;
  buf += sizeof(*dir);
  
  dir->magic = le_to_be_32((uint32_t) CSMAGIC_CODEDIRECTORY);
  dir->length = le_to_be_32((uint32_t) sizeof(*dir) + filename_size + num_blocks * SHA256_SIZE);
  dir->version = le_to_be_32((uint32_t) CS_SUPPORTSEXECSEG);
  dir->flags = le_to_be_32((uint32_t)CS_ADHOC | CS_LINKER_SIGNED);
  dir->hash_offset = le_to_be_32((uint32_t) sizeof(*dir) + filename_size);
  //printf("HASH offset es := %d\n", (unsigned int)((uint32_t) sizeof(*dir) + filename_size));
  dir->ident_offset = le_to_be_32((uint32_t) sizeof(*dir));
  dir->n_code_slots = le_to_be_32((uint32_t) num_blocks);
  dir->code_limit = le_to_be_32((uint32_t) offset_of_code_signature_data);
  dir->hash_size = le_to_be_8((uint8_t) SHA256_SIZE);
  dir->hash_type = le_to_be_8((uint8_t) CS_HASHTYPE_SHA256);
  dir->page_size = le_to_be_8((uint8_t) LOG2(system_page_size));			 // log2(page size in bytes)
  dir->exec_seg_base = le_to_be_64((uint64_t) offset_of_exec_segment); 		 // offset of executable segment
  dir->exec_seg_limit = le_to_be_64((uint64_t) size_of_exec_segment);		 // limit of executable segment
  //if (ctx.output_type == MH_EXECUTE)
  dir->exec_seg_flags = le_to_be_64((uint64_t)  CS_EXECSEG_MAIN_BINARY);

  //memcpy(buf, filename.data(), filename.size());
  memcpy(buf, output_file->filename, strlen(output_file->filename));
  buf += filename_size;

  // Compute a hash value for each block.
  //auto compute_hash = [&](i64 i) {
	//u8 *start = ctx.buf + i * E::page_size;
	//u8 *end = ctx.buf + std::min<i64>((i + 1) * E::page_size, this->hdr.offset);
	//sha256_hash(start, end - start, buf + i * SHA256_SIZE);
  //};

  //for (int64_t i = 0; i < num_blocks; i += 1024) {
	//int64_t j = min(num_blocks, i + 1024);
	//tbb::parallel_for(i, j, compute_hash);
	
	size_t last_value_of_k;
	
  for (int64_t i = 0; i < num_blocks; i += 1024) {

    int64_t j = min(num_blocks, i + 1024);

    for (int64_t k = i; k < j; k++) {	
      	uint8_t *start = output_file->buffer + k * system_page_size; 
      	uint8_t *end = output_file->buffer + min((k + 1) * system_page_size,
                                               offset_of_code_signature_data);
											   
		//printf("Escribiendo en %p\n", (void*)buf + k * SHA256_SIZE);
		//printf("Cuanto %zu\n", (size_t)(end - start));
		//printf("Desde %p\n", (void*)(start));
		//printf("Hasta %p\n\n", (void*)(end));
        CC_SHA256(start, end - start, buf + k * SHA256_SIZE);
		
		last_value_of_k = k;
    }
  }
  /*
  #define ARRAY_SIZE 176
  const uint8_t array[ARRAY_SIZE] = {
	0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x5f, 0x00, 0x12, 0x00, 0x00, 0x00,
	0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x90, 0x7f, 0x00, 0x00, 0x02, 0x5f, 0x6d, 0x68, 0x5f,
	0x65, 0x78, 0x65, 0x63, 0x75, 0x74, 0x65, 0x5f, 0x68, 0x65, 0x61, 0x64, 0x65, 0x72, 0x00, 0x09,
	0x6d, 0x61, 0x69, 0x6e, 0x00, 0x0d, 0x00, 0x00, 0x90, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00, 0x0f, 0x01, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x16, 0x00, 0x00, 0x00, 0x0f, 0x01, 0x00, 0x00, 0x90, 0x3f, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x5f, 0x5f, 0x6d, 0x68, 0x5f, 0x65, 0x78, 0x65, 0x63, 0x75, 0x74, 0x65, 0x5f, 0x68,
	0x65, 0x61, 0x64, 0x65, 0x72, 0x00, 0x5f, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x00, 0x00, 0x00, 0x00
  };
  
  uint8_t *start = output_file->buffer + last_value_of_k * system_page_size; 
  printf("last_value_of_k %zu\n", (last_value_of_k));
  printf("Desde %p\n", (void*)(start));
  uint8_t rest[176] = {0xFF};
  memcpy(rest, start, 176);
  
  if (memcmp(rest, array, 176) != 0) {
	  printf("son distintossss\n");
  }
  FILE* fptr_rest = create_file("./bin/averaverrrrr");
  size_t written = fwrite(rest, 176, 1, fptr_rest);
  if (written < 1) {
	  fprintf(stderr, "Error while writing the rest into the file.\n");
	  exit(1);
  }
  
  uint8_t *end = output_file->buffer + min((last_value_of_k + 1) * system_page_size,
										 offset_of_code_signature_data);
										 
  
  //CC_SHA256(array, 176, buf + last_value_of_k * SHA256_SIZE);
  //CC_SHA256(rest, 176, buf + last_value_of_k * SHA256_SIZE);
*/

  /*for (int64_t i = 0; i < 5; i++) {
	uint8_t* start = buf_start + i * system_page_size;
	long len = system_page_size;
	  																						// ctx->buf apuntaria al inicio de todo, del file entero!!!!!! lpm
	CC_SHA256(start, len, buf + i * SHA256_SIZE);
  }*/
  
  msync(output_file->buffer, output_file->filesize, MS_INVALIDATE);													/// MALLLLL este buf deberia ser ctx->buf
  
//#if __APPLE__
	// Calling msync() with MS_ASYNC speeds up the following msync()
	// with MS_INVALIDATE.
	//if (ctx.output_file->is_mmapped)
	//  msync(ctx.buf + i * E::page_size, 1024 * E::page_size, MS_ASYNC);
//#endif
  //}

/*
  // A LC_UUID load command may also contain a crypto hash of the
  // entire file. We compute its value as a tree hash.
  if (ctx.arg.uuid == UUID_HASH) {
	u8 uuid[SHA256_SIZE];
	sha256_hash(ctx.buf + this->hdr.offset, this->hdr.size, uuid);

	// Indicate that this is UUIDv4 as defined by RFC4122.
	uuid[6] = (uuid[6] & 0b00001111) | 0b01010000;
	uuid[8] = (uuid[8] & 0b00111111) | 0b10000000;

	memcpy(ctx.uuid, uuid, 16);

	// Rewrite the load commands to write the updated UUID and
	// recompute code signatures for the updated blocks.
	ctx.mach_hdr.copy_buf(ctx);

	for (i64 i = 0; i * E::page_size < ctx.mach_hdr.hdr.size; i++)
	  compute_hash(i);
  }
  */

/*
#if __APPLE__
  // If an executable's pages have been created via an mmap(), the output
  // file will fail for the code signature verification because the macOS
  // kernel wrongly assume that the pages may be mutable after the code
  // verification, though it is actually impossible after munmap().
  //
  // In order to workaround the issue, we call msync() to invalidate all
  // mmapped pages.
  //
  // https://openradar.appspot.com/FB8914231
  if (ctx.output_file->is_mmapped) {
	Timer t2(ctx, "msync", &t);
	msync(ctx.buf, ctx.output_file->filesize, MS_INVALIDATE);
  }
#endif
*/
}

void write_dyld_chained_fixups(struct OutputFile* output_file, size_t offset_of_dyld_chained_fixups_in_linkedit_segment) {
	
	uint8_t* buf = output_file->buffer + offset_of_dyld_chained_fixups_in_linkedit_segment;
	memset(buf, 0, sizeof(struct dyld_chained_fixups_header) + sizeof(struct dyld_chained_starts_in_image) + 20);
	
	struct dyld_chained_fixups_header* header = (struct dyld_chained_fixups_header *)buf;
	header->fixups_version = 0;
	header->starts_offset = 0x20;
	header->imports_offset = 0x30;
	header->symbols_offset = 0x30;
	header->imports_count = 0;
	header->imports_format = 1;
	header->symbols_format = 0;
	
	buf += sizeof(struct dyld_chained_fixups_header);
	
	struct dyld_chained_starts_in_image* idk = (struct dyld_chained_starts_in_image *)buf;
	idk->seg_count = 0;
	uint32_t aThree[1] = {3};
	memcpy(idk->seg_info_offset, aThree, sizeof(uint32_t));
	
	buf += sizeof(struct dyld_chained_starts_in_image);
	
	// I really don't understand what should follow the struct dyld_chained_starts_in_image but I know it's all zeros
	uint8_t zeros[20] = {
	  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00
	};
	memcpy(buf, zeros, 20 * sizeof(uint8_t));
	
}

void write_dyld_exports_trie(struct OutputFile* output_file, size_t offset_of_dyld_exports_trie_in_linkedit_segment) {
	
	uint8_t* buf = output_file->buffer + offset_of_dyld_exports_trie_in_linkedit_segment;
	memset(buf, 0, 48);
	
	const uint8_t exports_trie[48] = {
	  0x00, 0x01, 0x5f, 0x00, 0x12, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x90,
	  0x7f, 0x00, 0x00, 0x02, 0x5f, 0x6d, 0x68, 0x5f, 0x65, 0x78, 0x65, 0x63, 0x75, 0x74, 0x65, 0x5f,
	  0x68, 0x65, 0x61, 0x64, 0x65, 0x72, 0x00, 0x09, 0x6d, 0x61, 0x69, 0x6e, 0x00, 0x0d, 0x00, 0x00
	};
	
	memcpy(buf, exports_trie, 48 * sizeof(uint8_t));
}

void write_function_starts(struct OutputFile* output_file, size_t offset_of_function_starts) {
	
	uint8_t* buf = output_file->buffer + offset_of_function_starts;
	
	struct some_struct_to_represent_function_starts {
		uint32_t offset_one;
		uint32_t offset_two;
	};
	
	memset(buf, 0, sizeof(struct some_struct_to_represent_function_starts ));
	
	struct some_struct_to_represent_function_starts* function_start = (struct some_struct_to_represent_function_starts* )buf;
	function_start->offset_one = 0x7f90;
	function_start->offset_two = 0;
}


FILE* generate_handwritten_executable_file(void)
{
	// Create the file itself:
	char* filename = "handwritten_small_exec";
	char* filename_full_path = "./bin/handwritten_small_exec";
	
	struct OutputFile* output_file = malloc(sizeof(struct OutputFile));
	output_file->filename = malloc(strlen(filename));
	strcpy(output_file->filename, filename);
	output_file->filesize = 16864; //16848;
	
	FILE* fptr_out = create_file(filename_full_path);
	if (!fptr_out) {
		fprintf(stderr, "Error while creating file.\n");
		exit(1);
	}
	output_file->fptr = fptr_out;
	output_file->buffer = calloc(output_file->filesize, sizeof(uint8_t));
	output_file->total_size_for_load_cmds = 0;
	
	size_t file_offset = 0;
	// Put the header:
	write_macho_header(output_file, &file_offset);
	
	// Put the first load command (zero):
	write_load_command_zero(output_file, &file_offset);
	
	
	// Put the first load command (text):
	size_t offset_of_exec_segment;
	size_t size_of_exec_segment;
	write_load_command_one(output_file, &file_offset, &offset_of_exec_segment, &size_of_exec_segment);
	
	size_t file_offset_for_text_contents;
	// Put the first text section:
	// - First put the struct of the section itself:
	write_section_text(output_file, &file_offset, &file_offset_for_text_contents);
	// - And then the contents of the section:
	#define CODE_SIZE 24
	uint8_t* buf = output_file->buffer + file_offset_for_text_contents;
	memset(buf, 0, CODE_SIZE);
	uint8_t code[CODE_SIZE] = {
	  0xff, 0x43, 0x00, 0xd1, 0xe0, 0x0f, 0x00, 0xb9, 0xe1, 0x03, 0x00, 0xf9, 0x00, 0x00, 0x80, 0x52,
	  0xff, 0x43, 0x00, 0x91, 0xc0, 0x03, 0x5f, 0xd6
	};
	memcpy(buf, code, CODE_SIZE);
	
	// And now the unwind info:
	size_t file_offset_unwind_info;
	// - First put the struct of the section itself:
	write_section_unwind_info(output_file, &file_offset, &file_offset_unwind_info);
	// - And then the contents of the section:
	#define UNWIND_INFO_SIZE 88
	buf = output_file->buffer + file_offset_unwind_info;
	memset(buf, 0, UNWIND_INFO_SIZE);
	uint8_t unwind_info[88] = {
	  0x01, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00,
	  0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x90, 0x3f, 0x00, 0x00,
	  0x40, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0xa8, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x03, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x01, 0x00, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
	  0x00, 0x10, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00
	};
	memcpy(buf, unwind_info, CODE_SIZE);
	
	// Put the second load command (linkedit):
	write_load_command_two(output_file, &file_offset);
	// Put the third command:
	size_t offset_of_dyld_chained_fixups_in_linkedit_segment;
	write_load_command_three(output_file, &file_offset, &offset_of_dyld_chained_fixups_in_linkedit_segment);
	// Put the fourth command:
	size_t offset_of_dyld_exports_trie_in_linkedit_segment;
	write_load_command_four(output_file, &file_offset, &offset_of_dyld_exports_trie_in_linkedit_segment);
	// Put the fifth command:
	size_t file_offset_for_string_table;
	size_t size_of_string_table;
	size_t file_offset_for_symbol_table;
	size_t number_of_symbols;
	write_load_command_five(output_file, &file_offset, &file_offset_for_string_table,
							 &size_of_string_table, &file_offset_for_symbol_table, &number_of_symbols);
	// Put the sixth command:
	write_load_command_six(output_file, &file_offset);
	
	// Put the seventh command:
	write_load_command_seven(output_file, &file_offset);
	// For the seventh command we have to allocate an string, for its name:
	buf = output_file->buffer + file_offset;
	char* name = "/usr/lib/dyld";
	size_t name_size_aligned = align_to(strlen(name) + 1, 16);
	memset(buf, 0, name_size_aligned);
	memcpy(buf, name, strlen(name));
	file_offset += name_size_aligned;

	// Put the eight command:
	write_load_command_eight(output_file, &file_offset);
	
	// Put the nineth command:
	write_load_command_nine(output_file, &file_offset);
	
	// The nineth command expects to find a list of platforms and tools values following it, 
	// concretely this means that after the struct build_version_command that describes it,
	// there would be an array of ntools elements where each element is a struct build_tool_version
	// In this case ntools = 1, so:
	buf = output_file->buffer + file_offset;
	memset(buf, 0, sizeof(struct build_tool_version));
	struct build_tool_version* tool_version = (struct build_tool_version*)buf;
	tool_version->tool = TOOL_LD;
	tool_version->version = (1053 << 16) | (12 << 8);  	// version 1053.12   =>  X.Y.Z is encoded in nibbles xxxx.yy.zz  => X = 1053 Y = 12
	// As the struct build_tool_version has size multiple of 8, there is no need to add padding.
	// Move the file_offset to skip the struct build_tool_version:
	file_offset += sizeof(struct build_tool_version);

	// Put the tenth command:
	write_load_command_ten(output_file, &file_offset);
	
	// Put the eleventh command:
	write_load_command_eleven(output_file, &file_offset);
	
	// Put the twelveth command:
	write_load_command_twelve(output_file, &file_offset);
	// For the twelveth command we have to allocate an string, for its name:
	buf = output_file->buffer + file_offset;
	name = "/usr/lib/libSystem.B.dylib";
	name_size_aligned = align_to(strlen(name) + 1, 16);
	memset(buf, 0, name_size_aligned);
	memcpy(buf, name, strlen(name));
	file_offset += name_size_aligned;
	
	// Put the thirteenth command:
	size_t offset_of_function_starts;
	write_load_command_thirteen(output_file, &file_offset, &offset_of_function_starts);
	// Put the fourteenth command:
	write_load_command_fourteen(output_file, &file_offset);
	// Put the fifteenth command:
	size_t offset_for_code_sign = 16560;
	size_t size_of_code_sign = compute_size_for_code_signature_section(output_file->filename, offset_for_code_sign);
	write_load_command_fifteen(output_file, &file_offset, offset_for_code_sign, size_of_code_sign);
	
	// Now that we know the total size of all load commands update the total size for all load commands
	update_macho_header_total_size_for_load_cmds(output_file);
	
	// Put the string table
	write_string_table(output_file, file_offset_for_string_table, size_of_string_table);
	// Put the symbol table
	write_symbol_table(output_file, file_offset_for_symbol_table, number_of_symbols);
	
	write_dyld_chained_fixups(output_file, offset_of_dyld_chained_fixups_in_linkedit_segment);
	
	write_dyld_exports_trie(output_file, offset_of_dyld_exports_trie_in_linkedit_segment);
	
	write_function_starts(output_file, offset_of_function_starts);
	
	// Put the code signature. We need to have written the whole binary for this step, so we do it last:
	write_signature(output_file, offset_for_code_sign, size_of_code_sign, offset_of_exec_segment, size_of_exec_segment);
	
	// Finally write the file itself
	dump_whole_buffer_into_file(output_file);
	
	// So it has the same permissions as small_c_program
	if (chmod("./bin/handwritten_small_exec", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
		perror("chmod");
	}
	
	
	return fptr_out;
}



int main(int argc, char** argv) 
{
	if (argc != 2) {
		printf("For now only ONE mach-o file supported\n");
		printf("Usage: %s <mach-o file>\n", argv[0]);
		exit(1);
	}
	FILE* fptr = open_macho_file(argv[1], "rb");
	struct MachoHandle* macho_handle = build_macho_handle(fptr, argv[1]);
	if (macho_handle->filetype != RelocatableObjectFile) {
		printf("Error: Only relocatable object files supported\n");
		exit(1);
	}

	printf("macho_header mide: %zu\n", sizeof(struct mach_header_64));

	assign_raw_data_to_each_section(macho_handle);

	#if DEBUG
	printf("Debugging...\n");
	print_file_type(macho_handle);
	print_load_commands(macho_handle);
	print_segments_and_sections(macho_handle);
	dump_raw_data_of_section_to_file(macho_handle, "__text");
	dump_raw_data_of_section_to_file(macho_handle, "__compact_unwind__LD");
	#endif

	struct Builder* builder = new_builder();
	//struct SegmentHandle* input_text_segment = get_segment_handle_with_segment_name(macho_handle->segments, SEG_TEXT);
	// For some reason the __TEXT segment in relocatable object files doesn't have its name set
	// But we know it's the first one so:
	struct SegmentHandle* input_text_segment = get_segment_handle_at(macho_handle->segments, 0);
	if (!input_text_segment) {
		fprintf(stderr, "Main: Error getting the %s segment\n", SEG_TEXT);
		exit(1);
	}
	builder_set_input_text_segment_handle(builder, input_text_segment);
	
	if (!build_segment_text(builder)) {
		fprintf(stderr, "Main: Error while building the output text segment\n");
		exit(1);
	}


	struct OutputFile* output_file = writer_generate_executable_file("my_own_small_exec", builder);

	//FILE* file_ptr = generate_handwritten_executable_file();
	
	
	//fclose(file_ptr);
	
	return 0;
}
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

/////////////////////////////////////////////////////////


struct OffsetAndSize {
	uint32_t offset;
    uint32_t size;

	OffsetAndSize();
	OffsetAndSize(uint32_t, uint32_t);
};

// The offsets dict gives us for a given Load Command, the offset and the size in the memory region.
// This way it's easier to get a Load Command from the region, because we would know how much to read.
struct LoadCommandsRegion {
	Byte* region;
	//std::map<LoadCommandInfo, OffsetAndSize, LoadCommandInfoCompare> offsets;
	std::map<std::string, OffsetAndSize> offsets;

	LoadCommandsRegion();
};


/*
The offsets dictionary will have:
Key: an string representing the CMD, if it's LC_SEGMENT_64 then it will be LC_SEGMENT_66:__TEXT for example.
Value: The OffsetAndSize corresponding to where in the memory region this load command begins and for how many bytes occupies (its size).

*/



/////////////////////////////////////////////////////////


struct LoadCommand {
    uint32_t cmd;		
	uint32_t cmdsize;
};
/*
struct symtab_command {
	uint32_t	cmd;		// LC_SYMTAB 
	uint32_t	cmdsize;	// sizeof(struct symtab_command)
	uint32_t	symoff;		// symbol table offset
	uint32_t	nsyms;		// number of symbol table entries
	uint32_t	stroff;		// string table offset
	uint32_t	strsize;	// string table size in bytes
};
*/
struct SymTabCommand : LoadCommand {
    uint32_t	symoff;		// symbol table offset
	uint32_t	nsyms;		// number of symbol table entries
	uint32_t	stroff;		// string table offset
	uint32_t	strsize;	// string table size in bytes
};

/* The symbol table will have symtab_cmd->nsyms of these structs:
	struct nlist_64 {
		union {
			uint32_t n_strx;  // index into the string table 
		} n_un;
		uint8_t n_type;       // type flag, see below 
		uint8_t n_sect;       // section number or NO_SECT 
		uint16_t n_desc;      // see <mach-o/stab.h> 
		uint64_t n_value;     // value of this symbol (or stab offset) 
	};
    To obtain them we need to move to symtab_cmd->symoff

#define	N_UNDF	0x0		// undefined, n_sect == NO_SECT 
#define	N_ABS	0x2		// absolute, n_sect == NO_SECT 
#define	N_SECT	0xe		// defined in section number n_sect 
#define	N_PBUD	0xc		// prebound undefined (defined in a dylib) 
#define N_INDR	0xa		// indirect 
*/

struct StringTableEntry {
    char* string;
    size_t index_into_table;
};

class StringTable {
    friend class Debugger;
    //struct SymbolTableEntry* entries;
    //size_t number_of_entries;
	//size_t symbol_table_size; 		// total size in bytes
    public:
        StringTable();
        size_t get_string_table_size();

        std::vector<StringTableEntry> entries;
        struct strtab_command* strtab_cmd;
};

enum SymbolType {UNDF, ABS, SECT, PBUD, INDR};

struct SymbolTableEntry {
    size_t index_into_string_table;
    SymbolType type;
    uint8_t section_number;
    uint16_t desc;   
	uint64_t value;
};

class SymbolTable {
    friend class Debugger;
    //struct SymbolTableEntry* entries;
    //size_t number_of_entries;
	//size_t symbol_table_size; 		// total size in bytes
    public:
        SymbolTable();
        size_t get_symbol_table_size();

        std::vector<SymbolTableEntry> entries;
        struct symtab_command* symtab_cmd;
        StringTable strtab;
};

SymbolType get_own_symbol_table_entry_type(uint8_t n_type);


/*
 * The 64-bit segment load command indicates that a part of this file is to be
 * mapped into a 64-bit task's address space.  If the 64-bit segment has
 * sections then section_64 structures directly follow the 64-bit segment
 * command and their size is reflected in cmdsize.
*/
struct SegmentCommand64 : LoadCommand {
    char		segname[16];	// segment name 
	uint64_t	vmaddr;		    // memory address of this segment 
	uint64_t	vmsize;		    // memory size of this segment 
	uint64_t	fileoff;	    // file offset of this segment 
	uint64_t	filesize;	    // amount to map from the file 
	vm_prot_t	maxprot;	    // maximum VM protection 
	vm_prot_t	initprot;	    // initial VM protection 
	uint32_t	nsects;		    // number of sections in segment
	uint32_t	flags;		    // flags 
};


/*
 * A segment is made up of zero or more sections.  Non-MH_OBJECT files have
 * all of their segments with the proper sections in each, and padded to the
 * specified segment alignment when produced by the link editor.  The first
 * segment of a MH_EXECUTE and MH_FVMLIB format file contains the mach_header
 * and load commands of the object file before its first section.  The zero
 * fill sections are always last in their segment (in all formats).  This
 * allows the zeroed segment padding to be mapped into memory where zero fill
 * sections might be. The gigabyte zero fill sections, those with the section
 * type S_GB_ZEROFILL, can only be in a segment with sections of this type.
 * These segments are then placed after all other segments.
 *
 * The MH_OBJECT format has all of its sections in one segment for
 * compactness.  There is no padding to a specified segment boundary and the
 * mach_header and load commands are not part of the segment.
 *
 * Sections with the same section name, sectname, going into the same segment,
 * segname, are combined by the link editor.  The resulting section is aligned
 * to the maximum alignment of the combined sections and is the new section's
 * alignment.  The combined sections are aligned to their original alignment in
 * the combined section.  Any padded bytes to get the specified alignment are
 * zeroed.
 *
 * The format of the relocation entries referenced by the reloff and nreloc
 * fields of the section structure for mach object files is described in the
 * header file <reloc.h>.
 */
struct Section64 {
	char		sectname[16];	// name of this section 
	char		segname[16];	// segment this section goes in 
	uint64_t	addr;			// memory address of this section
	uint64_t	size;			// size in bytes of this section
	uint32_t	offset;			// file offset of this section
	uint32_t	align;			// section alignment (power of 2)
	uint32_t	reloff;			// file offset of relocation entries
	uint32_t	nreloc;			// number of relocation entries
	uint32_t	flags;			// flags (section type and attributes)
	uint32_t	reserved1;		// reserved (for offset or index)
	uint32_t	reserved2;		// reserved (for count or sizeof)
	uint32_t	reserved3;		// reserved 
};

struct SectionWithPayload {
	Section64* section;
	Byte* payload;
};


struct SegmentHandle {
	SegmentCommand64* segcmd;
	std::vector<SectionWithPayload> sections;

	SegmentHandle();
};



////////

/*
 * The linkedit_data_command contains the offsets and sizes of a blob
 * of data in the __LINKEDIT segment.  
 *
struct linkedit_data_command {
    uint32_t	cmd;		/ LC_CODE_SIGNATURE, LC_SEGMENT_SPLIT_INFO,
                                LC_FUNCTION_STARTS, LC_DATA_IN_CODE,
				   				LC_DYLIB_CODE_SIGN_DRS,
				   				LC_LINKER_OPTIMIZATION_HINT,
				   				LC_DYLD_EXPORTS_TRIE, or
				   				LC_DYLD_CHAINED_FIXUPS.
    uint32_t	cmdsize;	/ sizeof(struct linkedit_data_command) 
    uint32_t	dataoff;	/ file offset of data in __LINKEDIT segment 
    uint32_t	datasize;	/ file size of data in __LINKEDIT segment
};
*/
struct LinkeditDataCommand : LoadCommand {
	uint32_t	dataoff;	// file offset of data in __LINKEDIT segment
    uint32_t	datasize;	// file size of data in __LINKEDIT segment
};

struct LinkeditCommandWithPayload {
	LinkeditDataCommand* command;
	Byte* payload;

	LinkeditCommandWithPayload();
};

////////

/*
uint32_t	cmd;		// LC_BUILD_VERSION
uint32_t	cmdsize;	// sizeof(struct build_version_command) plus ntools * sizeof(struct build_tool_version)
*/
struct BuildVersionCommand : LoadCommand {
	uint32_t	platform;	// platform 
    uint32_t	minos;		// X.Y.Z is encoded in nibbles xxxx.yy.zz
    uint32_t	sdk;		// X.Y.Z is encoded in nibbles xxxx.yy.zz
    uint32_t	ntools;		// number of tool entries following this
};
/*
 * The build_version_command contains the min OS version on which this
 * binary was built to run for its platform.  The list of known platforms and
 * tool values following it.
 *
struct build_tool_version {
    uint32_t	tool;		// enum for the tool
    uint32_t	version;	// version number of the tool
};
*/
struct BuildVersion {
	BuildVersionCommand* command;
	std::vector<struct build_tool_version> tool_versions;

	BuildVersion();
};



//////////////////////////////////////////////

extern std::map<uint32_t, std::string> macroToString;

//////////////////////////////////////////////


#endif
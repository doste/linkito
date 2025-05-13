#ifndef LOAD_COMMANDS_H
#define LOAD_COMMANDS_H


#include "Common.h"



struct OffsetAndSize {
	uint64_t offset;
    uint64_t size;

	OffsetAndSize();
	OffsetAndSize(uint64_t, uint64_t);
};

// The offsets dict gives us for a given Load Command, the offset and the size in the memory region.
// This way it's easier to get a Load Command from the region, because we would know how much to read.
struct LoadCommandsRegion {
	Byte* region;
	std::map<std::string, OffsetAndSize> offsets;

	LoadCommandsRegion();
};
/*
The offsets dictionary will have:
Key: an string representing the CMD, if it's LC_SEGMENT_64 then it will be LC_SEGMENT_66:__TEXT for example.
Value: The OffsetAndSize corresponding to where in the memory region this load command begins and for how many bytes occupies (its size).
*/


////////////////////////////////////////////////////////////////////////////////////////////


struct LoadCommand {
    uint32_t cmd;		// type of load command
	uint32_t cmdsize;	// total size of command in bytes 
};

////////////////////////////////////////////////////////////////////////////////////////////

struct LoadCommandHandle {
    LoadCommand* load_command;

	LoadCommandHandle();
	virtual void print() const = 0;

};

////////////////////////////////////////////////////////////////////////////////////////////

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

struct SymTabCommandHandle : LoadCommandHandle {
	virtual void print() const;

	SymTabCommandHandle();
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

    public:
        SymbolTable();
        size_t get_symbol_table_size();

        std::vector<SymbolTableEntry> entries;
		SymTabCommandHandle* symtab_command_handle;

        StringTable strtab;
};

SymbolType get_own_symbol_table_entry_type(uint8_t n_type);

////////////////////////////////////////////////////////////////////////////////////////////

/*
 * This is the second set of the symbolic information which is used to support
 * the data structures for the dynamically link editor.
 *
 * The original set of symbolic information in the symtab_command which contains
 * the symbol and string tables must also be present when this load command is
 * present.  When this load command is present the symbol table is organized
 * into three groups of symbols:
 *	local symbols (static and debugging symbols) - grouped by module
 *	defined external symbols - grouped by module (sorted by name if not lib)
 *	undefined external symbols (sorted by name if MH_BINDATLOAD is not set,
 *	     			    and in order the were seen by the static
 *				    linker if MH_BINDATLOAD is set)
 * In this load command there are offsets and counts to each of the three groups
 * of symbols.
 *
 * This load command contains a the offsets and sizes of the following new
 * symbolic information tables:
 *	table of contents
 *	module table
 *	reference symbol table
 *	indirect symbol table
 * The first three tables above (the table of contents, module table and
 * reference symbol table) are only present if the file is a dynamically linked
 * shared library.  For executable and object modules, which are files
 * containing only one module, the information that would be in these three
 * tables is determined as follows:
 * 	table of contents - the defined external symbols are sorted by name
 *	module table - the file contains only one module so everything in the
 *		       file is part of the module.
 *	reference symbol table - is the defined and undefined external symbols
 *
 * For dynamically linked shared library files this load command also contains
 * offsets and sizes to the pool of relocation entries for all sections
 * separated into two groups:
 *	external relocation entries
 *	local relocation entries
 * For executable and object modules the relocation entries continue to hang
 * off the section structures.
 
struct dysymtab_command {
    uint32_t cmd;		/* LC_DYSYMTAB
    uint32_t cmdsize;	/* sizeof(struct dysymtab_command)
    /*
     * The symbols indicated by symoff and nsyms of the LC_SYMTAB load command
     * are grouped into the following three groups:
     *    local symbols (further grouped by the module they are from)
     *    defined external symbols (further grouped by the module they are from)
     *    undefined symbols
     *
     * The local symbols are used only for debugging.  The dynamic binding
     * process may have to use them to indicate to the debugger the local
     * symbols for a module that is being bound.
     *
     * The last two groups are used by the dynamic binding process to do the
     * binding (indirectly through the module table and the reference symbol
     * table when this is a dynamically linked shared library file).
    
    uint32_t ilocalsym;	/* index to local symbols
    uint32_t nlocalsym;	/* number of local symbols
    uint32_t iextdefsym;/* index to externally defined symbols
    uint32_t nextdefsym;/* number of externally defined symbols
    uint32_t iundefsym;	/* index to undefined symbols
    uint32_t nundefsym;	/* number of undefined symbols
    /*
     * For the for the dynamic binding process to find which module a symbol
     * is defined in the table of contents is used (analogous to the ranlib
     * structure in an archive) which maps defined external symbols to modules
     * they are defined in.  This exists only in a dynamically linked shared
     * library file.  For executable and object modules the defined external
     * symbols are sorted by name and is use as the table of contents.
    
    uint32_t tocoff;	/* file offset to table of contents 
    uint32_t ntoc;	/* number of entries in table of contents
    /*
     * To support dynamic binding of "modules" (whole object files) the symbol
     * table must reflect the modules that the file was created from.  This is
     * done by having a module table that has indexes and counts into the merged
     * tables for each module.  The module structure that these two entries
     * refer to is described below.  This exists only in a dynamically linked
     * shared library file.  For executable and object modules the file only
     * contains one module so everything in the file belongs to the module.
    
    uint32_t modtaboff;	/* file offset to module table
    uint32_t nmodtab;	/* number of module table entries
    /*
     * To support dynamic module binding the module structure for each module
     * indicates the external references (defined and undefined) each module
     * makes.  For each module there is an offset and a count into the
     * reference symbol table for the symbols that the module references.
     * This exists only in a dynamically linked shared library file.  For
     * executable and object modules the defined external symbols and the
     * undefined external symbols indicates the external references.
	
    uint32_t extrefsymoff;	/* offset to referenced symbol table
    uint32_t nextrefsyms;	/* number of referenced symbol table entries
    /*
     * The sections that contain "symbol pointers" and "routine stubs" have
     * indexes and (implied counts based on the size of the section and fixed
     * size of the entry) into the "indirect symbol" table for each pointer
     * and stub.  For every section of these two types the index into the
     * indirect symbol table is stored in the section header in the field
     * reserved1.  An indirect symbol table entry is simply a 32bit index into
     * the symbol table to the symbol that the pointer or stub is referring to.
     * The indirect symbol table is ordered to match the entries in the section.

    uint32_t indirectsymoff; /* file offset to the indirect symbol table
    uint32_t nindirectsyms;  /* number of indirect symbol table entries 
    /*
     * To support relocating an individual module in a library file quickly the
     * external relocation entries for each module in the library need to be
     * accessed efficiently.  Since the relocation entries can't be accessed
     * through the section headers for a library file they are separated into
     * groups of local and external entries further grouped by module.  In this
     * case the presents of this load command who's extreloff, nextrel,
     * locreloff and nlocrel fields are non-zero indicates that the relocation
     * entries of non-merged sections are not referenced through the section
     * structures (and the reloff and nreloc fields in the section headers are
     * set to zero).
     *
     * Since the relocation entries are not accessed through the section headers
     * this requires the r_address field to be something other than a section
     * offset to identify the item to be relocated.  In this case r_address is
     * set to the offset from the vmaddr of the first LC_SEGMENT command.
     * For MH_SPLIT_SEGS images r_address is set to the the offset from the
     * vmaddr of the first read-write LC_SEGMENT command.
     *
     * The relocation entries are grouped by module and the module table
     * entries have indexes and counts into them for the group of external
     * relocation entries for that the module.
     *
     * For sections that are merged across modules there must not be any
     * remaining external relocation entries for them (for merged sections
     * remaining relocation entries must be local).

    uint32_t extreloff;	/* offset to external relocation entries
    uint32_t nextrel;	/* number of external relocation entries
    /*
     * All the local relocation entries are grouped together (they are not
     * grouped by their module since they are only used if the object is moved
     * from it staticly link edited address).
    uint32_t locreloff;	/* offset to local relocation entries
    uint32_t nlocrel;	/* number of local relocation entries
};
*/

struct DySymTabCommand : LoadCommand {
	uint32_t ilocalsym;			// index to local symbols
    uint32_t nlocalsym;			// number of local symbols
    uint32_t iextdefsym;		// index to externally defined symbols
    uint32_t nextdefsym;		// number of externally defined symbols
    uint32_t iundefsym;			// index to undefined symbols
    uint32_t nundefsym;			// number of undefined symbols
	uint32_t tocoff;			// file offset to table of contents
    uint32_t ntoc;				// number of entries in table of contents
	uint32_t modtaboff;			// file offset to module table
    uint32_t nmodtab;			// number of module table entries
	uint32_t extrefsymoff;		// offset to referenced symbol table
    uint32_t nextrefsyms;		// number of referenced symbol table entries
	uint32_t indirectsymoff; 	// file offset to the indirect symbol table
	uint32_t nindirectsyms; 	// number of indirect symbol table entries
	uint32_t extreloff;			// offset to external relocation entries
	uint32_t nextrel;			// number of external relocation entries
	uint32_t locreloff;			// offset to local relocation entries
	uint32_t nlocrel;			// number of local relocation entries
};

struct DySymTabHandle : LoadCommandHandle {
	DySymTabHandle();
	virtual void print() const;
};

////////////////////////////////////////////////////////////////////////////////////////////

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

// (Section64 is not a LoadCommand)
struct SectionHandle {
	Section64* section;
	Byte* payload;      // The size of this payload is given by section->size

    SectionHandle();
	void print() const;
};


struct SegmentHandle : LoadCommandHandle {
	std::vector<SectionHandle*> sections;

    std::string segname;

	SegmentHandle();
	virtual void print() const;
};



////////////////////////////////////////////////////////////////////////////////////////////

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

struct LinkeditDataCommandHandle : LoadCommandHandle {
	LinkeditDataCommand* load_command;
	Byte* payload;

	LinkeditDataCommandHandle();
	virtual void print() const;
};

struct DyldChainedFixupsCommand : LinkeditDataCommand {};
struct DyldChainedFixupsCommandHandle : LinkeditDataCommandHandle {
    //Its load_command field will be of type DyldChainedFixupsCommand*;
    DyldChainedFixupsCommandHandle();
};

struct FunctionStartsCommand : LinkeditDataCommand {};
struct FunctionStartsCommandHandle : LinkeditDataCommandHandle {
    //Its load_command field will be of type FunctionStartsCommand*;
    FunctionStartsCommandHandle();
};

struct DataInCodeCommand : LinkeditDataCommand {};
struct DataInCodeCommandHandle : LinkeditDataCommandHandle {
    //Its load_command field will be of type DataInCodeCommandHandle*;
    DataInCodeCommandHandle();
};

struct CodeSignatureCommand : LinkeditDataCommand {};
struct CodeSignatureCommandHandle : LinkeditDataCommandHandle {
    //Its load_command field will be of type CodeSignatureCommand*;
    CodeSignatureCommandHandle();
};

struct ExportsTrieCommand : LinkeditDataCommand {};
struct ExportsTrieCommandHandle : LinkeditDataCommandHandle {
    //Its load_command field will be of type ExportsTrieCommand*;
    ExportsTrieCommandHandle();
};

////////////////////////////////////////////////////////////////////////////////////////////

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
struct BuildVersionHandle : LoadCommandHandle {
	std::vector<struct build_tool_version> tool_versions;

	BuildVersionHandle();
	virtual void print() const;
};

////////////////////////////////////////////////////////////////////////////////////////////

/*
 * Dynamicly linked shared libraries are identified by two things.  The
 * pathname (the name of the library as found for execution), and the
 * compatibility version number.  The pathname must match and the compatibility
 * number in the user of the library must be greater than or equal to the
 * library being used.  The time stamp is used to record the time a library was
 * built and copied into user so it can be use to determined if the library used
 * at runtime is exactly the same as used to built the program.
 
struct dylib {
    union lc_str  name;				/* library's path name
    uint32_t timestamp;				/* library's build time stamp
    uint32_t current_version;		/* library's current version number
    uint32_t compatibility_version;	/* library's compatibility vers number
};

 * A dynamically linked shared library (filetype == MH_DYLIB in the mach header)
 * contains a dylib_command (cmd == LC_ID_DYLIB) to identify the library.
 * An object that uses a dynamically linked shared library also contains a
 * dylib_command (cmd == LC_LOAD_DYLIB, LC_LOAD_WEAK_DYLIB, or
 * LC_REEXPORT_DYLIB) for each library it uses.
struct dylib_command {
	uint32_t	cmd;			/* LC_ID_DYLIB, LC_LOAD_{,WEAK_}DYLIB, LC_REEXPORT_DYLIB
	uint32_t	cmdsize;		/* includes pathname string 
	struct dylib	dylib;		/* the library identification
};
*/
struct LoadDylibCommand : LoadCommand {
	struct dylib dylib;		// the library identification
};

struct LoadDylibCommandHandle : LoadCommandHandle {
    char* library_path_name;            // Easier to access it

	LoadDylibCommandHandle();
	virtual void print() const;
};

////////////////////////////////////////////////////////////////////////////////////////////

/*
 * The source_version_command is an optional load command containing
 * the version of the sources used to build the binary.
 
struct source_version_command {
    uint32_t  cmd;		/* LC_SOURCE_VERSION 
    uint32_t  cmdsize;	/* 16 
    uint64_t  version;	/* A.B.C.D.E packed as a24.b10.c10.d10.e10 
};
*/
struct SourceVersionCommand : LoadCommand {
	uint64_t  version;		// A.B.C.D.E packed as a24.b10.c10.d10.e10
};

struct SourceVersionCommandHandle : LoadCommandHandle {

	SourceVersionCommandHandle();
	virtual void print() const;
};

////////////////////////////////////////////////////////////////////////////////////////////

/*
 * The uuid load command contains a single 128-bit unique random number that
 * identifies an object produced by the static link editor.
struct uuid_command {
    uint32_t	cmd;		/* LC_UUID
    uint32_t	cmdsize;	/* sizeof(struct uuid_command)
    uint8_t	uuid[16];		/* the 128-bit uuid
};
*/
struct UuidCommand : LoadCommand {
	uint8_t	uuid[16];		// the 128-bit uuid
};

struct UuidCommandHandle : LoadCommandHandle {
	UuidCommandHandle();
	virtual void print() const;
};

////////////////////////////////////////////////////////////////////////////////////////////

/*
 * The entry_point_command is a replacement for thread_command.
 * It is used for main executables to specify the location (file offset)
 * of main().  If -stack_size was used at link time, the stacksize
 * field will contain the stack size need for the main thread.
struct entry_point_command {
    uint32_t  cmd;			/* LC_MAIN only used in MH_EXECUTE filetypes
    uint32_t  cmdsize;		/* 24
    uint64_t  entryoff;		/* file (__TEXT) offset of main()
    uint64_t  stacksize;	/* if not zero, initial stack size
};
*/
struct EntryPointCommand : LoadCommand {
	uint64_t  entryoff;		// file (__TEXT) offset of main()
    uint64_t  stacksize;	// if not zero, initial stack size
};

struct EntryPointCommandHandle : LoadCommandHandle {
	EntryPointCommandHandle();
	virtual void print() const;
};


////////////////////////////////////////////////////////////////////////////////////////////

/*
 * A program that uses a dynamic linker contains a dylinker_command to identify
 * the name of the dynamic linker (LC_LOAD_DYLINKER).  And a dynamic linker
 * contains a dylinker_command to identify the dynamic linker (LC_ID_DYLINKER).
 * A file can have at most one of these.
 * This struct is also used for the LC_DYLD_ENVIRONMENT load command and
 * contains string for dyld to treat like environment variable.

struct dylinker_command {
	uint32_t	cmd;			/* LC_ID_DYLINKER, LC_LOAD_DYLINKER or LC_DYLD_ENVIRONMENT
	uint32_t	cmdsize;		/* includes pathname string
	union lc_str    name;		/* dynamic linker's path name
};
*/
struct LoadDyLinkerCommand : LoadCommand {
	union lc_str name;		// dynamic linker's path name
};

struct LoadDyLinkerCommandHandle : LoadCommandHandle {
    char* pathname;                 // Easier to access it

	LoadDyLinkerCommandHandle();
	virtual void print() const;
};


////////////////////////////////////////////////////////////////////////////////////////////

#endif
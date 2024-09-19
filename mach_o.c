
#include <mach/machine.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <mach-o/loader.h>
#include <sys/_types/_size_t.h>
#include <mach-o/nlist.h>
#include <inttypes.h>
#include "./step1/not_really_cs_blobs.h"

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
#define	MH_MAGIC_64 0xfeedfacf 		// the 64-bit mach magic number
*/

struct MachoHandle {
    FILE* fptr;
    struct mach_header_64 header;
};

FILE* open_macho_file(const char *pathname, const char *mode) {
    FILE*  fptr = fopen(pathname, mode);
    assert(fptr != NULL);
    return fptr;
}

struct MachoHandle* build_macho_handle(FILE* fptr) {
    struct MachoHandle* handle = (struct MachoHandle*)malloc(sizeof(struct MachoHandle));

    handle->fptr = fptr;

    size_t items_read = fread(&(handle->header), sizeof(struct mach_header_64), 1, fptr);
    if (items_read != 1) {
        printf("fread failed.\n");
        exit(1);
    }



    return handle;
}


// DEBUG
 void print_file_type(struct MachoHandle* handle)
{
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

void inspect_header(struct MachoHandle* handle)
{
    //uint32_t	magic;			// mach magic number identifier
    printf("magic: %d\n", handle->header.magic);
	//cpu_type_t	cputype;		// cpu specifier  (typedef int	cpu_type_t;)
	printf("cputype: %d\n", handle->header.cputype);
	//cpu_subtype_t	cpusubtype;	// machine specifier  (typedef int	cpu_subtype_t;)
	printf("cpusubtype: %d\n", handle->header.cpusubtype);
	//uint32_t	filetype;		// type of file
	printf("filetype: ");
	print_file_type(handle);
	//uint32_t	ncmds;			// number of load commands
	printf("ncmds: %d\n", handle->header.ncmds);
	//uint32_t	sizeofcmds;		// the size of all the load commands
	printf("sizeofcmds: %d\n", handle->header.sizeofcmds);
	//uint32_t	flags;			// flags
	printf("flags: %d\n", handle->header.flags);
    
    
    // Aver que flags tiene prendido:    
    if (handle->header.flags & MH_NOUNDEFS) {
        printf("tiene prendido MH_NOUNDEFS\n");
    }
    if (handle->header.flags & MH_INCRLINK) {
        printf("tiene prendido MH_INCRLINK\n");
    }
    if (handle->header.flags & MH_DYLDLINK) {
        printf("tiene prendido MH_DYLDLINK\n");
    }
    if (handle->header.flags & MH_BINDATLOAD) {
        printf("tiene prendido MH_BINDATLOAD\n");
    }
    if (handle->header.flags & MH_PREBOUND) {
        printf("tiene prendido MH_PREBOUND\n");
    }
    if (handle->header.flags & MH_SPLIT_SEGS) {
        printf("tiene prendido MH_SPLIT_SEGS\n");
    }
    if (handle->header.flags & MH_LAZY_INIT) {
        printf("tiene prendido MH_LAZY_INIT\n");
    }
    if (handle->header.flags & MH_TWOLEVEL) {
        printf("tiene prendido MH_TWOLEVEL\n");
    }
    if (handle->header.flags & MH_FORCE_FLAT) {
        printf("tiene prendido MH_FORCE_FLAT\n");
    }
    if (handle->header.flags & MH_NOMULTIDEFS) {
        printf("tiene prendido MH_NOMULTIDEFS\n");
    }
    if (handle->header.flags & MH_NOFIXPREBINDING) {
        printf("tiene prendido MH_NOFIXPREBINDING\n");
    }
    if (handle->header.flags & MH_NOMULTIDEFS) {
        printf("tiene prendido MH_NOMULTIDEFS\n");
    }
    if (handle->header.flags & MH_PREBINDABLE) {
        printf("tiene prendido MH_PREBINDABLE\n");
    }
    if (handle->header.flags & MH_ALLMODSBOUND) {
        printf("tiene prendido MH_ALLMODSBOUND\n");
    }
    if (handle->header.flags & MH_SUBSECTIONS_VIA_SYMBOLS) {
        printf("tiene prendido MH_SUBSECTIONS_VIA_SYMBOLS\n");
    }
    if (handle->header.flags & MH_WEAK_DEFINES) {
        printf("tiene prendido MH_WEAK_DEFINES\n");
    }
    if (handle->header.flags & MH_BINDS_TO_WEAK) {
        printf("tiene prendido MH_BINDS_TO_WEAK\n");
    }
    if (handle->header.flags & MH_CANONICAL) {
        printf("tiene prendido MH_CANONICAL\n");
    }
    if (handle->header.flags & MH_ALLOW_STACK_EXECUTION) {
        printf("tiene prendido MH_ALLOW_STACK_EXECUTION\n");
    }
    if (handle->header.flags & MH_ROOT_SAFE) {
        printf("tiene prendido MH_ROOT_SAFE\n");
    }
    if (handle->header.flags & MH_SETUID_SAFE) {
        printf("tiene prendido MH_SETUID_SAFE\n");
    }
    if (handle->header.flags & MH_NO_REEXPORTED_DYLIBS) {
        printf("tiene prendido MH_NO_REEXPORTED_DYLIBS\n");
    }
    if (handle->header.flags & MH_PIE) {
        printf("tiene prendido MH_PIE\n");
    }
    if (handle->header.flags & MH_NO_HEAP_EXECUTION) {
        printf("tiene prendido MH_NO_HEAP_EXECUTION\n");
    }
    
    
	//uint32_t	reserved;		// reserved; 64 bit only
	printf("reserved: %d\n", handle->header.reserved);

}

// Print the value of each field in each nlist_64 corresponding to each symbol in the symbol table
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
void print_symbol_table_fields_of_entries(FILE* fptr, uint32_t symbol_table_offset, uint32_t number_of_symbols) {
    
    // We'll move the fptr so we need to save its value
    long saved_fptr = ftell(fptr);
    
    // Move the fptr so it points to the beginning of the symbol table
    // First to the beginning of the file
    rewind(fptr); 
    // So we can add just the offset of the symbol table
    fseek(fptr, symbol_table_offset, SEEK_SET); 
    
    for (uint32_t i = 0; i < number_of_symbols; i++) {
        struct nlist_64 symbol_table_entry = {0};
        size_t items_read = fread(&symbol_table_entry, sizeof(struct nlist_64), 1, fptr);
        if (items_read != 1) {
            printf("fread failed.\n");
            exit(1);
        }
        
        printf("struct nlist_64\n");
        printf("    n_un.n_strx: %u\n", symbol_table_entry.n_un.n_strx);
        printf("    n_type: %hhu\n", symbol_table_entry.n_type);
        printf("    n_sect: %hhu\n", symbol_table_entry.n_sect);
        printf("    n_desc: 0x%02X\n", symbol_table_entry.n_desc);
        printf("    n_value: %" PRIu64 "\n", symbol_table_entry.n_value);
        printf("\n");
    }
    
    // Restore the value of the fptr
    if (fseek(fptr, saved_fptr, SEEK_SET) != 0) printf("fseek error\n");
}

void print_string_table(FILE* fptr, uint32_t string_table_offset, uint32_t size_of_string_table) {
    // We'll move the fptr so we need to save its value
    long saved_fptr = ftell(fptr);
    
    // Move the fptr so it points to the beginning of the symbol table
    // First to the beginning of the file
    rewind(fptr); 
    // So we can add just the offset of the symbol table
    fseek(fptr, string_table_offset, SEEK_SET); 
    
    char* buffer = malloc(size_of_string_table);
    size_t items_read = fread(buffer, size_of_string_table, 1, fptr);
    if (items_read != 1) {
        printf("fread failed.\n");
        exit(1);
    }
    
    printf("String Table:");
    char* ptr = buffer;
    size_t i = 0;
    bool something_was_printed;
    while (i < size_of_string_table) {
        something_was_printed = false;
        
        if (*ptr == '\0') {
            ptr++;
        }
        
        while(*ptr != '\0') {
            something_was_printed = true;
            printf("%c", *ptr);
            ptr++;
        }
        if (something_was_printed) {
            printf("\n");
        }
        i++;
    }
    printf("\n");
    
    
    // Restore the value of the fptr
    if (fseek(fptr, saved_fptr, SEEK_SET) != 0) printf("fseek error\n");
    
    free(buffer);
}

struct falopaUno {
    uint8_t a;
    uint64_t b;
};
struct falopaDos {
    char c;
    uint64_t d;
};

void averaverrr(void) {
    char* filename_full_path = "./AVERhandwritten_small_exec";
    FILE* file_ptr = fopen(filename_full_path, "wb+");
    uint8_t* buf = calloc(32, sizeof(uint8_t));
    uint8_t* buf_start = buf;
    
    struct falopaUno* falopita = (struct falopaUno *)buf;
    buf += sizeof(*falopita);
    
    falopita->a = 3;
    falopita->b = 10;
    
    struct falopaDos* falopitaDos = (struct falopaDos *)buf;
    
    falopitaDos->c = 0x33;
    falopitaDos->d = 0xFF;
    
    
    size_t written = fwrite(buf_start, sizeof(uint8_t), 32, file_ptr);
    if (written < 32) {
        fprintf(stderr, "Error while writing the dfjiofjigjfi into the file.\n");
        exit(1);
    }
}


int main(int argc, char** argv, char** envp) {

    if (argc != 2) {
        printf("Usage: ./macho_parser <macho_file_to_load>\n");
        exit(1);
    }

    FILE* fptr = open_macho_file(argv[1], "rb");

    // lo parseamos y nos construimos el handle
    struct MachoHandle* handle = build_macho_handle(fptr);

    assert(handle->header.magic == MH_MAGIC_64);
    assert(handle->header.cputype == CPU_TYPE_ARM64);
    
    printf("struct mach_header_64 mide %zu\n", sizeof(struct mach_header_64));
    
    printf("struct section_64 mide %zu\n", sizeof(struct section_64));
    
    printf("struct segment_command_64 mide %zu\n", sizeof(struct segment_command_64));
    
    printf("struct dylib_command mide %zu\n", sizeof(struct dylib_command));
    
    printf("struct nlist_64  mide %zu\n", sizeof(struct nlist_64));
    
    printf("CS_CodeDirectory  mide %zu\n", sizeof(CS_CodeDirectory));
    
    
    // I want to know what the value of each field in each nlist_64 corresponding to each symbol 
    print_symbol_table_fields_of_entries(handle->fptr, 16496, 2);
    
    print_string_table(handle->fptr, 16528, 32);


    inspect_header(handle);
    
    
    averaverrr();



    return 0;
}

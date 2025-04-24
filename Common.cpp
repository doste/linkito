#include "Common.h"
#include "LoadCommands.h"

uint64_t align_to(uint64_t val, uint64_t align) {
    if (align == 0)
      return val;
    return (val + align - 1) & ~(align - 1);
  }

File::File() : filename(nullptr), filesize(0), fptr(nullptr) {}

File::File(char* filename, size_t filesize, FILE* fptr) :
         filename(filename), filesize(filesize), fptr(fptr) {}

size_t File::get_file_size(FILE* fptr) {
    fseek(fptr, 0L, SEEK_END);
    size_t size = ftell(fptr);
    rewind(fptr);
    return size;
}

void File::fill_buffer() {
    buffer = (Byte*)calloc(filesize, sizeof(Byte));
    size_t items_read = fread(buffer, sizeof(Byte), filesize, fptr);
    if (items_read != filesize) {
        fprintf(stderr, "fill_buffer: Error while freading the entire file.\n");
        exit(1);
    }
}

void read_macho_header(FILE* fptr, struct mach_header_64* header) {
    size_t items_read = fread(header, sizeof(struct mach_header_64), 1, fptr);
    if (items_read != 1) {
        fprintf(stderr, "Error while reading Mach-o header.\n");
        exit(1);
    }
    if (header->magic != MH_MAGIC_64) {
        fprintf(stderr, "Error: Not a Mach-o file!\n");
        exit(1);
    }
}

FILE* open_macho_file(const char *pathname) {
    FILE* fptr = fopen(pathname, "rb");
    if (fptr == NULL) {
        fprintf(stderr, "Error while opening: %s.\n", pathname);
        exit(1);
    }
    assert(fptr != NULL);
    return fptr;
}


SymbolType get_own_symbol_table_entry_type(uint8_t n_type) {
    switch (n_type & N_TYPE) {
        case N_UNDF: {
            return UNDF;
        }
        case N_ABS: {
            return ABS;
        }
        case N_SECT: {
            return SECT;
        }
        case N_PBUD: {
            return PBUD;
        }
        case N_INDR: {
            return INDR;
        }
        default:
            fprintf(stderr, "Error: get_own_symbol_table_entry_type \n Invalid symbol type: 0x%x\n",
                    n_type & N_TYPE);
            exit(1);
        }
}


////////////////////////////////////////////////////

std::map<uint32_t, std::string> macroToString {
    {LC_ROUTINES_64, "LC_ROUTINES_64"},
    {LC_SEGMENT_64, "LC_SEGMENT_64"},
    {LC_LOAD_WEAK_DYLIB, "LC_LOAD_WEAK_DYLIB"},
    {LC_UUID, "LC_UUID"},
    {LC_RPATH, "LC_RPATH"},
    {LC_REQ_DYLD, "LC_REQ_DYLD"},
    {LC_CODE_SIGNATURE, "LC_CODE_SIGNATURE"},
    {LC_SEGMENT_SPLIT_INFO, "LC_SEGMENT_SPLIT_INFO"},
    {LC_REEXPORT_DYLIB, "LC_REEXPORT_DYLIB"},
    {LC_LAZY_LOAD_DYLIB, "LC_LAZY_LOAD_DYLIB"},
    {LC_ENCRYPTION_INFO, "LC_ENCRYPTION_INFO"},
    {LC_DYLD_INFO, "LC_DYLD_INFO"},
    {LC_DYLD_INFO_ONLY, "LC_DYLD_INFO_ONLY "},
    {LC_LOAD_UPWARD_DYLIB, "LC_LOAD_UPWARD_DYLIB "},
    {LC_VERSION_MIN_MACOSX, "LC_VERSION_MIN_MACOSX"},
    {LC_VERSION_MIN_IPHONEOS, "LC_VERSION_MIN_IPHONEOS "},
    {LC_FUNCTION_STARTS, "LC_FUNCTION_STARTS"},
    {LC_DYLD_ENVIRONMENT, "LC_DYLD_ENVIRONMENT"},
    {LC_MAIN, "LC_MAIN"},
    {LC_DATA_IN_CODE, "LC_DATA_IN_CODE"},
    {LC_SOURCE_VERSION, "LC_SOURCE_VERSION"},
    {LC_DYLIB_CODE_SIGN_DRS, "LC_DYLIB_CODE_SIGN_DRS"},
    {LC_ENCRYPTION_INFO_64, "LC_ENCRYPTION_INFO_64"},
    {LC_LINKER_OPTION, "LC_LINKER_OPTION"},
    {LC_LINKER_OPTIMIZATION_HINT, "LC_LINKER_OPTIMIZATION_HINT"},
    {LC_VERSION_MIN_TVOS, "LC_VERSION_MIN_TVOS"},
    {LC_VERSION_MIN_WATCHOS, "LC_VERSION_MIN_WATCHOS"},
    {LC_NOTE, "LC_NOTE"},
    {LC_BUILD_VERSION, "LC_BUILD_VERSION"},
    {LC_DYLD_EXPORTS_TRIE, "LC_DYLD_EXPORTS_TRIE"},
    {LC_DYLD_CHAINED_FIXUPS, "LC_DYLD_CHAINED_FIXUPS"},
    {LC_SEGMENT, "LC_SEGMENT"},
    {LC_SYMTAB, "LC_SYMTAB"},
    {LC_SYMSEG, "LC_SYMSEG"},
    {LC_THREAD, "LC_THREAD"},
    {LC_UNIXTHREAD, "LC_UNIXTHREAD"},
    {LC_LOADFVMLIB, "LC_LOADFVMLIB"},
    {LC_IDFVMLIB, "LC_IDFVMLIB"},
    {LC_IDENT,"LC_IDENT"},
    {LC_FVMFILE, "LC_FVMFILE"},
    {LC_PREPAGE, "LC_PREPAGE"},
    {LC_DYSYMTAB, "LC_DYSYMTAB"},
    {LC_LOAD_DYLIB, "LC_LOAD_DYLIB "},
    {LC_ID_DYLIB, "LC_ID_DYLIB"},
    {LC_LOAD_DYLINKER, "LC_LOAD_DYLINKER"},
    {LC_ID_DYLINKER, "LC_ID_DYLINKER"},
    {LC_PREBOUND_DYLIB, "LC_PREBOUND_DYLIB"},
    {LC_ROUTINES, "LC_ROUTINES"},
    {LC_SUB_FRAMEWORK ,"LC_SUB_FRAMEWORK"},
    {LC_SUB_UMBRELLA ,"LC_SUB_UMBRELLA"},
    {LC_SUB_CLIENT ,"LC_SUB_CLIENT"},
    {LC_SUB_LIBRARY ,"LC_SUB_LIBRARY"},
    {LC_TWOLEVEL_HINTS, "LC_TWOLEVEL_HINTS"},
    {LC_PREBIND_CKSUM, "LC_PREBIND_CKSUM"},
};
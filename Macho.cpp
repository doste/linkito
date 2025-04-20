#include "Macho.h"
#include <set>
#include <tuple>

Macho::Macho() {}

Macho::Macho(char* filename, const char* pathname) {
    FILE* fptr = open_macho_file(pathname);
    read_macho_header(fptr, &this->header);
    switch (this->header.filetype) {
        case MH_OBJECT:
            this->filetype = RelocatableObjectFile;
            break;
        case MH_EXECUTE:
            this->filetype = ExecutableFile;
            break;
        case MH_DYLIB:
            this->filetype = DynamicLibrary;
            break;
        default:
            fprintf(stderr, "Error: Not supported filetype\n");
            exit(1);
    }

    this->file = File(filename, File::get_file_size(fptr), fptr);
    this->file.fill_buffer();
    this->segment_handles = std::vector<SegmentHandle*>();
    this->linkedit_data_handles = std::vector<LinkeditDataCommandHandle*>();

    //this->buildLoadCommandsMemoryRegion();
    
}







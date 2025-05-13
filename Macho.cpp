#include "Macho.h"
#include <set>
#include <tuple>

Macho::Macho() {
    this->header = new MachHeader64();
    this->segment_handles = new std::vector<SegmentHandle*>();
    this->linkedit_data_handles = new std::vector<LinkeditDataCommandHandle*>();
}


Macho::Macho(char* filename) {
    this->header = new MachHeader64();
    FILE* fptr = open_macho_file(filename);
    read_macho_header(fptr, this->header);
    switch (this->header->filetype) {
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
    this->segment_handles = new std::vector<SegmentHandle*>();
    this->linkedit_data_handles = new std::vector<LinkeditDataCommandHandle*>();

    //this->buildLoadCommandsMemoryRegion();
    
}

SegmentHandle* Macho::getSegmentHandleForSegmentNamed(std::string segname) {
    for (SegmentHandle* seg : *this->segment_handles) {
        if (seg->segname == segname) {
            return seg;
        }
    }
    return nullptr;
}

LinkeditDataCommandHandle* Macho::getLinkeditDataCmdHandleForCmd(uint32_t cmd) {
    for (LinkeditDataCommandHandle* linkedit_cmd_handle : *this->linkedit_data_handles) {
        if (linkedit_cmd_handle->load_command->cmd == cmd) {
            return linkedit_cmd_handle;
        }
    }
    return nullptr;
}







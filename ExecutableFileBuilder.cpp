#include "ExecutableFileBuilder.h"


struct MemoryRegion {
    Byte* data;
    OffsetAndSize offset_and_size;
};

struct MemoryRegionManager {

    Byte* block_of_memory;
    std::vector<MemoryRegion> regions;
    MemoryRegionManager();

    void appendMemoryRegion(MemoryRegion mem_reg);
};

ExecutableFileBuilder::ExecutableFileBuilder() {}

ExecutableFileBuilder::ExecutableFileBuilder(Macho input_macho) : input_macho(input_macho) {}

// For now the output header will be equal to the input header, except for the following fields:
//  - filetype
//  - ncmds
//  - sizeofcmds
//  - flags
void ExecutableFileBuilder::buildHeader() {
    struct mach_header_64 input_header = this->input_macho.header;
    struct mach_header_64 output_header = input_header;
    output_header.filetype = MH_EXECUTE;
    // The other fields will be updated later when the whole file is built.


}

void ExecutableFileBuilder::buildExecutableFile() {

}
#include "ExecutableFileBuilder.h"
#include <algorithm>

MemoryRegion::MemoryRegion() {}

MemoryRegion::MemoryRegion(uint32_t size) {
    this->data = (Byte*)malloc(sizeof(Byte) * size);
    this->offset_and_size = OffsetAndSize(0, size);
}

void MemoryRegion::fillMemoryRegion(void* data) {
    memcpy(this->data, data, this->offset_and_size.size);
}

////////////////////////////////////////////////////////////////////////////////////////////

MemoryRegionManager::MemoryRegionManager() {
    this->offset_and_size_of_block = OffsetAndSize(0, 0);  // An initial value.
    this->capacity = 64;   // An initial value.
    this->block_of_memory = (Byte*)malloc(sizeof(Byte) * this->capacity);
    this->regions = std::vector<MemoryRegion*>();
}


void MemoryRegionManager::appendMemoryRegion(MemoryRegion* mem_reg) {
    if (mem_reg->offset_and_size.size > this->capacity) {
        // If it doesn't fit, we need to allocate more memory.
        this->capacity += std::max(mem_reg->offset_and_size.size, (uint32_t)64);
        this->block_of_memory = (Byte*)realloc(this->block_of_memory, this->capacity);
    }

    uint32_t offset_of_this_mem_reg_in_the_block_of_memory = this->offset_and_size_of_block.offset;
    memcpy(this->block_of_memory + this->offset_and_size_of_block.offset, mem_reg->data, mem_reg->offset_and_size.size);
    this->offset_and_size_of_block.offset += mem_reg->offset_and_size.size;
    this->offset_and_size_of_block.size += mem_reg->offset_and_size.size;

    // Once a MemoryRegion is appended to the Manager, the fields of the MemoryRegion changes:
    //  - data is free'd . That data now resides in the block of memory of the Manager.
    //  - offset now is with respect to the block of memory of the Manager.
    //  - size remains the same.
    mem_reg->offset_and_size.offset = offset_of_this_mem_reg_in_the_block_of_memory;
    free(mem_reg->data);

    this->regions.push_back(mem_reg);
}


////////////////////////////////////////////////////////////////////////////////////////////

ExecutableFileBuilder::ExecutableFileBuilder() {}

ExecutableFileBuilder::ExecutableFileBuilder(Macho input_macho, MemoryRegionManager mem_reg_manager) : input_macho(input_macho), mem_reg_manager(mem_reg_manager) {}



// For now the output header will be equal to the input header, except for the following fields:
//  - filetype
//  - ncmds
//  - sizeofcmds
//  - flags
void ExecutableFileBuilder::buildHeader() {
    struct mach_header_64 output_header = this->input_macho.header;
    output_header.filetype = MH_EXECUTE;
    // The other fields will be updated later when the whole file is built.

    MemoryRegion* mem_reg_header = new MemoryRegion(sizeof(struct mach_header_64));
    mem_reg_header->fillMemoryRegion(&output_header);

    this->mem_reg_manager.appendMemoryRegion(mem_reg_header);

    std::cout << "mem_reg_header OFFSET: " << mem_reg_header->offset_and_size.offset << std::endl;
    std::cout << "mem_reg_header SIZE: " << mem_reg_header->offset_and_size.size << std::endl;
}





void ExecutableFileBuilder::buildPageZeroSegment() {
    SegmentHandle* seg_handle = new SegmentHandle();
    seg_handle->load_command = new SegmentCommand64();

    *seg_handle->load_command = (SegmentCommand64){ 
                                                    .segname = SEG_PAGEZERO,
                                                    .vmaddr	= 0x0000000000000000,
                                                    .vmsize	= 0x0000000100000000,
                                                    .fileoff = 0,
                                                    .filesize = 0,
                                                    .maxprot = 0,
                                                    .initprot = 0,
                                                    .nsects	= 0,
                                                    .flags = 0 };
    seg_handle->load_command->cmd = LC_SEGMENT_64;
    seg_handle->load_command->cmdsize = sizeof(SegmentCommand64);
    
    seg_handle->segname = SEG_PAGEZERO;

    MemoryRegion* mem_reg_seg_pagezero = new MemoryRegion(sizeof(SegmentCommand64));
    mem_reg_seg_pagezero->fillMemoryRegion(seg_handle->load_command);

    this->mem_reg_manager.appendMemoryRegion(mem_reg_seg_pagezero);

    std::cout << "mem_reg_seg_pagezero OFFSET: " << mem_reg_seg_pagezero->offset_and_size.offset << std::endl;
    std::cout << "mem_reg_seg_pagezero SIZE: " << mem_reg_seg_pagezero->offset_and_size.size << std::endl;
}

void ExecutableFileBuilder::buildExecutableFile() {
    this->buildHeader();
    this->buildPageZeroSegment();
}
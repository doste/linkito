#ifndef EXECUTABLE_FILE_BUILDER_H
#define EXECUTABLE_FILE_BUILDER_H

#include "Macho.h"


struct MemoryRegion {
    Byte* data;
    OffsetAndSize offset_and_size;

    MemoryRegion();
    MemoryRegion(uint32_t);

    void fillMemoryRegion(void*);
};

struct MemoryRegionManager {
    Byte* block_of_memory;
    OffsetAndSize offset_and_size_of_block;      // Size of block_of_memory and offset where to start with the next copy.
    uint32_t capacity;                           // Capacity of the block of memory.

    std::vector<MemoryRegion*> regions;
    MemoryRegionManager();

    void appendMemoryRegion(MemoryRegion* mem_reg);

};

class ExecutableFileBuilder {
    friend class Debugger;

    public:
        ExecutableFileBuilder();
        ExecutableFileBuilder(Macho, MemoryRegionManager);

        void buildExecutableFile();

    private:
        Macho input_macho;
        MemoryRegionManager mem_reg_manager;

        void buildHeader();
        void buildPageZeroSegment();
};


#endif
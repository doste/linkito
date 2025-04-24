#ifndef EXECUTABLE_FILE_BUILDER_H
#define EXECUTABLE_FILE_BUILDER_H

#include "Macho.h"

#define INITIAL_CAPACITY 64

/*
A MemoryRegionManager manages two different regions of memory: an upper one and a lower one.
The upper one is where all the load commands will reside.
The lower one is where all the corresponding 'payloads' will be.
For example, the __text section's struct Section64 will be in the upper one, but the contents
of the section will be in the lower one.

Then, once all the pieces are in place, the two regions will be merged into one, filling with zeros
the middle. That will be the final executable file.

Each 'MemoryRegion' in turn are composed of 'MemoryBlock's.
For example each LoadCommmand will be build using a MemoryBlock and then appending it to the Region.
*/

struct MemoryBlock {
    Byte* data;
    OffsetAndSize offset_and_size;      // Offset with respect to the beginning of the Region containing it.

    MemoryBlock();
    MemoryBlock(uint64_t);

    void fillMemoryBlock(void*);
};

struct MemoryRegion {
    Byte* data;
    OffsetAndSize offset_and_size;      // Offset and size will be always the same, it's where the writing would start in the next memcpy.
    uint64_t capacity;

    MemoryRegion();

    std::vector<MemoryBlock*> blocks;

    void appendMemoryBlock(MemoryBlock* mem_block);

    void debug();

};

struct MemoryRegionManager {

    MemoryRegionManager();


    MemoryRegion* upperMemoryRegion;
    MemoryRegion* lowerMemoryRegion;

    Byte* mergeRegions();

    void debug();
};

class ExecutableFileBuilder {
    friend class Debugger;

    public:
        ExecutableFileBuilder();
        ExecutableFileBuilder(Macho);

        void buildExecutableFile();
        Byte* wholeFile;        // Temporarie. Just to test.

        void debugMemoryRegionManager();

    private:
        Macho input_macho;
        MemoryRegionManager* mem_reg_manager;
        Macho output_macho;

        void buildLoadCommand(LoadCommandHandle* load_command_handle, LoadCommand* load_command, uint32_t cmd);

        void buildHeader();
        void buildSegments();
            void buildPageZeroSegment();
            void buildTextSegment();
            void buildLinkeditSegment();
        void buildTextSectionPayload();
        void buildLinkeditDataCommands();
        void buildLinkeditDataCommand(LinkeditDataCommandHandle* handle, LinkeditDataCommand* linkedit_data_load_command, uint32_t cmd);
            void buildDyldChainedFixupsCommand();
            void buildExportsTrieCommand();
            void buildDataInCodeCommand();
            void buildFunctionStartCommand();
            void buildCodeSignatureCommand();

        void buildBuildVersionCommand();
        void buildSymbolTable();
        void buildDySymbolTable();
        void buildStringTable();
        
        void buildDyLinkerCommand();
        void buildEntryPointCommand();
        void buildUuidCommand();
        void buildSourceVersionCommand();
        void buildLoadDylibCommandHandle();

};


#endif
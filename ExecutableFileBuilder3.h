
#ifndef EXECUTABLE_FILE_BUILDER3_H
#define EXECUTABLE_FILE_BUILDER3_H

#include "Macho.h"
#include <optional>

#define INITIAL_CAPACITY 64


struct MemoryBlock {
    std::string id;
    Byte* data{};
    OffsetAndSize offset_and_size;      // Offset with respect to the beginning of the Region containing it.

    MemoryBlock();
    MemoryBlock(OffsetAndSize);
    MemoryBlock(Byte*, OffsetAndSize);

    void fillWithZeros();
};


struct MemoryRegion {
    Byte* data;
    OffsetAndSize offset_and_size;      // Offset and size will be always the same, it's where the writing would start in the next memcpy.
    uint64_t capacity;

    MemoryRegion();

    std::vector<MemoryBlock> blocks;
    void appendMemoryBlock(MemoryBlock* mem_block);
    void debug();
};

// The offsets dict gives us for a given Load Command, the offset and the size in the memory region.
// This way it's easier to get a Load Command from the region, because we would know how much to read.

// LoadCommandsRegion2 treats Section64 commands as if they were LoadCommands.
// So, the offsets dict also have entries for Section64 commands.
struct LoadCommandsRegion2 {
	MemoryRegion region;
	std::map<std::string, OffsetAndSize> offsets;

	LoadCommandsRegion2();
    void appendHeader(MachHeader64* header);
    void appendLoadCommand(LoadCommand* lc, uint64_t size_of_lc);
    void appendLoadCommandWithDataAfterIt(LoadCommand*, uint64_t, Byte*, uint64_t);
    void appendSectionCommand(Section64* sect_cmd);
};

struct MemoryRegionManager {

    MemoryRegionManager();


    LoadCommandsRegion2* loadCommandsMemoryRegion;
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
        Byte* wholeFile;

        void debug();

    private:
        Macho input_macho;
        Macho output_macho;
        MemoryRegionManager* mem_reg_manager;

        void buildHeader();
        void buildPageZeroSegment();
        void buildTextSegment();
        void buildLinkeditSegment();
        void buildLoadCommand(LoadCommandHandle*, uint32_t);
        void buildLoadCommandAppendingDataJustAfter(LoadCommandHandle*, uint32_t, Byte*, uint32_t);
        void buildLinkeditDataCommand(LinkeditDataCommandHandle*);
        void buildDyldChainedFixupsCommand();
        void buildExportsTrieCommand();
        void buildDataInCodeCommand();
        void buildFunctionStartCommand();
        void buildCodeSignatureCommand();
        void buildSymbolTable();
        void buildDySymbolTable();
        void buildDyLinkerCommand();
        void buildUuidCommand();
        void buildBuildVersionCommand();
        void buildSourceVersionCommand();
        void buildEntryPointCommand();
        void buildLoadDylibCommandHandle();


        void patchHeader();
        void patchLoadCommand(std::string, Byte*, uint64_t);
        void patchPageZeroSegment();
        void patchLinkeditDataCommand(uint32_t, uint32_t, uint32_t);
        void patchEntryPointCommand(uint64_t);

        std::optional<uint64_t> getIndexOfSegmentWithName(std::string);

        void debugOffsetsMap();
};


#endif
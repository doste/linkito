#ifndef EXECUTABLE_FILE_BUILDER_H
#define EXECUTABLE_FILE_BUILDER_H

#include "Macho.h"

#include <map>

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
/*
struct MemoryBlock {
    std::string id;
    Byte* data{};
    OffsetAndSize offset_and_size;      // Offset with respect to the beginning of the Region containing it.

    MemoryBlock();
    MemoryBlock(uint64_t);

    template <class T>
    void fillMemoryBlockT(T* data, std::string id);
 

    void fillMemoryBlock(void*);
    void fillMemoryBlock(void*, std::string id);
    void fillMemoryBlockWithZeros();
};

template <class T>
struct MemoryBlock {
    uint64_t id;
    T* data{};
    OffsetAndSize offset_and_size;      // Offset with respect to the beginning of the Region containing it.

    MemoryBlock();
    MemoryBlock(uint64_t);
    MemoryBlock(uint64_t size, std::string id);
    MemoryBlock(Byte* data, uint64_t size);

    void fillMemoryBlockT(T* data, std::string id);
 
    void fillMemoryBlock(void*);
    void fillMemoryBlock(void*, std::string id);
    void fillMemoryBlockWithZeros();
};





struct MemoryRegion {
    Byte* data;
    OffsetAndSize offset_and_size;      // Offset and size will be always the same, it's where the writing would start in the next memcpy.
    uint64_t capacity;

    MemoryRegion();

    //std::vector<MemoryBlock*> blocks;
    std::vector<MemoryBlock<Byte>> blocks;

    //std::map<std::string, MemoryBlock*> blocks_id_map;
    //std::map<std::string, MemoryBlock<Byte>*> blocks_id_map;
    std::map<std::string, uint64_t> blocks_id_map;  // Dictionary for blocks [id => offset in the region]

    //void appendMemoryBlock(MemoryBlock* mem_block);
    
    void appendMemoryBlock(MemoryBlock<Byte>* mem_block);

    uint64_t appendMemoryBlockToLayout(MemoryBlock<Byte> mem_block);

    //MemoryBlock* retrieveMemoryBlockById(std::string id);
    template <class T>
    MemoryBlock<T>* retrieveMemoryBlockById(std::string id);

    void f(MemoryBlock<Byte> h);
    void buildMemoryRegionFromLayout();
    void buildMemoryRegionFromLayout(MemoryBlock<Byte>*);
    
    MemoryBlock<Byte>& getMemoryBlockWithId(uint64_t id);


    template <class T>
    T** retrieveMemoryBlockByIdAndType(std::string id);

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

        void firstPass();
        void patchHeader(uint64_t);
        void patchPageZeroSegment(uint64_t);
        //void patchValues();
        void patchValues(std::map<std::string, uint64_t>);
        void patchEntryPointCommand();


        void buildExecutableFile();
        Byte* wholeFile;        // Temporarie. Just to test.

        void debugMemoryRegionManager();
        void testLoadCommandsMemoryRegionIsBuiltCorrectly();

    private:
        Macho input_macho;
        MemoryRegionManager* mem_reg_manager;
        Macho output_macho;

        void buildLoadCommand(LoadCommandHandle* load_command_handle, LoadCommand* load_command, uint32_t cmd, uint32_t cmdsize);
        void buildLoadCommandAppendingDataJustAfterOffset(LoadCommandHandle* load_command_handle,
                                                    LoadCommand* load_command,
                                                    uint32_t cmd, uint32_t cmdsize,
                                                    uint32_t ofsset_for_data, void* data_to_append, uint32_t size_of_data_to_append);
        
        
        uint64_t buildHeader();
        //MemoryBlock<Byte>* buildHeader();
        void buildSegments();
            uint64_t buildPageZeroSegment();
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
*/

#endif
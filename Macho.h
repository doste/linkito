#ifndef MACHO_H
#define MACHO_H

#include <stdio.h>
#include <stdlib.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <mach-o/reloc.h>
#include <cerrno>
#include <assert.h>
#include <iostream>
#include <vector>
#include <set>
#include <optional>
#include "Common.h"
#include "LoadCommands.h"

   
class Macho {
    friend class Debugger;
    friend class Tester;
    
    public:
        Macho(char* filename, const char* pathname);

        void buildLoadCommands();


    private:
    
        struct mach_header_64 header;
        macho_filetype filetype;
        File file;

        SymbolTable symtab;
        std::vector<SegmentHandle*> segment_handles;
        std::vector<LinkeditDataCommandHandle*> linkedit_data_handles;
        BuildVersionHandle* build_version_handle;
        LoadDyLinkerCommandHandle* load_dylinker_handle;
        EntryPointCommandHandle* entry_point_handle;
        UuidCommandCommandHandle* uuid_handle;
        SourceVersionCommandHandle* source_version_handle;
        LoadDylibCommandHandle* load_dylib_handle;
        LoadCommandsRegion load_commands_mem_region;


        void buildLoadCommandsMemoryRegion();
        std::vector<std::string> getSegmentLoadCommandsPresentInTheMap();

        // Building of load commands. Each of these is called by buildLoadCommands().
        void buildBuildVersionLoadCommand();
        void buildSymbolTable();
        void buildStringTable();
        void buildSegmentCommands();
        void buildLinkeditDataCommands();
        void buildDyLinkerCommand();
        void buildEntryPointCommand();
        void buildUuidCommand();
        void buildSourceVersionCommand();
        void buildLoadDylibCommandHandle();

        void assignPayloadsToSections();
        void assignPayloadsToLinkeditBlobs();



};

#endif
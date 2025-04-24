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
    friend class ExecutableFileBuilder;
    friend class MachoParser;
    
    public:
        
        Macho();
        Macho(char* filename);

        SegmentHandle* getSegmentHandleForSegmentNamed(std::string segname);

    private:
    
        struct mach_header_64 header;
        macho_filetype filetype;
        File file;

        // The Macho object itself is not responsible for setting all the following fields.
        // In the case of an input Macho, the MachoParser will be responsible for setting them up.
        // In the case of an output Macho, it will be the ExecutableFileBuilder.
        // The idea is that a Macho object can be used as both input or output.
        SymbolTable symtab;
        DySymTabHandle* dysymtab_handle;
        std::vector<SegmentHandle*>* segment_handles;
        std::vector<LinkeditDataCommandHandle*>* linkedit_data_handles;
        BuildVersionHandle* build_version_handle;
        LoadDyLinkerCommandHandle* load_dylinker_handle;
        EntryPointCommandHandle* entry_point_handle;
        UuidCommandCommandHandle* uuid_handle;
        SourceVersionCommandHandle* source_version_handle;
        LoadDylibCommandHandle* load_dylib_handle;
        LoadCommandsRegion load_commands_mem_region;
};

#endif
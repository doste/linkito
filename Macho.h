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

   
class Macho {
    friend class Debugger;
    
    public:
        Macho(char* filename, const char* pathname);
        void buildSegmentCommands();
        void buildSegmentCommandsDEP();   // X
        void buildLinkeditDataCommands(); 
        void buildLinkeditDataCommandsDEP(); // X

        void buildBuildVersionLoadCommand();
        void buildBuildVersionLoadCommandDEP(); // X
        void buildLoadCommands();

        void buildSymbolTable();
        void buildStringTable();
        void assignPayloadsToSections();
        void assignPayloadsToLinkeditBlobs();

        //void buildLoadCommandsMemoryRegion();
        //LoadCommandsRegion load_commands; // This should be private, now only to test it

    private:
    
        struct mach_header_64 header;
        macho_filetype filetype;
        File file;

        //LoadCommandsRegion load_commands;
        SymbolTable symtab;
        std::vector<SegmentHandle> segment_commands;
        std::vector<LinkeditCommandWithPayload> linkedit_data;
        BuildVersion build_version;

        std::vector<std::string> getSegmentLoadCommandsPresentInTheMap();

        void buildLoadCommandsMemoryRegion();
        LoadCommandsRegion load_commands;

};

#endif
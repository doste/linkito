#ifndef MACHO_PARSER_H
#define MACHO_PARSER_H

#include "Macho.h"

class MachoParser {
    
    public:
        MachoParser();
        MachoParser(char* filename);

        void buildLoadCommands();

        Macho* macho;

    private:

        void buildLoadCommandsMemoryRegion();
        std::vector<std::string> getSegmentLoadCommandsPresentInTheMap();

        // Building of load commands. Each of these is called by buildLoadCommands().
        void buildBuildVersionLoadCommand();
        void buildSymbolTable();
        void buildDySymbolTable();
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
#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <iostream>
#include "Common.h"
#include "Macho.h"

class Debugger {
    public:
        void debugMacho(Macho macho);
        void debugSymbolTable(Macho macho);
        void debugStringTable(Macho macho);
        void debugSegmentCommands(Macho macho);

        void dumpLinkeditPayloadsToFile(Macho macho);
        void dumpTextSectionToFile(Macho macho);

        void dumpBuildVersionCommandToFile(Macho macho);

        void dumpWholeLoadCommandsMemoryRegionToFile(Macho macho);
        void dumpLoadCommandsMemoryRegionToFile(Macho macho, uint32_t offset, uint32_t size);



        // Shoul be in a class Tester
        void testLoadCommandsMemoryRegionIsBuiltCorrectly(Macho macho);
};

#endif
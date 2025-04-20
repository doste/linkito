#ifndef TESTER_H
#define TESTER_H

#include "Macho.h"

class Tester {
    public:
        void testLoadCommandsMemoryRegionIsBuiltCorrectly(Macho);
        void printLoadCommands(Macho);
};

#endif
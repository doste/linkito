#include <stdio.h>
#include <stdlib.h>
#include <cerrno>
#include <assert.h>
#include <iostream>
#include "Macho.h"
#include "Debugger.h"



int main(int argc, char** argv) {
    if (argc != 2) {
		fprintf(stderr, "For now only ONE mach-o file supported\n");
		fprintf(stderr, "Usage: %s <mach-o file>\n", argv[0]);
		exit(1);
	}
	
	char* input_filename = argv[1];
    Macho macho = Macho(input_filename, input_filename);

    Debugger debugger = Debugger();
    //debugger.debugMacho(macho);

    macho.buildLoadCommands();

    //debugger.debugSymbolTable(macho);
    //debugger.debugStringTable(macho);
    //debugger.debugSegmentCommands(macho);

    //debugger.dumpLinkeditPayloadsToFile(macho);
    //debugger.dumpTextSectionToFile(macho);


    //debugger.dumpBuildVersionCommandToFile(macho);

    //debugger.dumpWholeLoadCommandsMemoryRegionToFile(macho);

    debugger.testLoadCommandsMemoryRegionIsBuiltCorrectly(macho);


    return 0;
}
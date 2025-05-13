#include <stdio.h>
#include <stdlib.h>
#include <cerrno>
#include <assert.h>
#include <iostream>
//#include "Macho.h"
#include "Debugger.h"
#include "Tester.h"
#include "MachoParser.h"

#include "ExecutableFileBuilder3.h"



int main(int argc, char** argv) {
    if (argc != 2) {
		fprintf(stderr, "For now only ONE mach-o file supported\n");
		fprintf(stderr, "Usage: %s <mach-o file>\n", argv[0]);
		exit(1);
	}
	
	char* input_filename = argv[1];
    //Macho macho = Macho(input_filename, input_filename);

    Debugger debugger = Debugger();
    Tester tester = Tester();

    MachoParser parser = MachoParser(input_filename);

    //debugger.debugMacho(macho);

    parser.buildLoadCommands();

    //debugger.debugSymbolTable(macho);
    //debugger.debugStringTable(macho);
    //debugger.debugSegmentCommands(macho);

    //debugger.dumpLinkeditPayloadsToFile(macho);
    //debugger.dumpTextSectionToFile(macho);


    //debugger.dumpBuildVersionCommandToFile(macho);

    parser.patchTextSeg();

    debugger.dumpWholeLoadCommandsMemoryRegionToFile(*parser.macho);

    tester.testLoadCommandsMemoryRegionIsBuiltCorrectly(*parser.macho);
    //tester.printLoadCommands(macho);

    ExecutableFileBuilder builder = ExecutableFileBuilder(*parser.macho);
    builder.buildExecutableFile();

    //builder.debug();

    debugger.dumpUpperMemoryRegionToFile(builder);
    
    //debugger.dumpWholeFileToFile(builder);

    //builder.debugMemoryRegionManager();

    //builder.testLoadCommandsMemoryRegionIsBuiltCorrectly();

    



    return 0;
}
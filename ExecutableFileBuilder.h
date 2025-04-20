#ifndef EXECUTABLE_FILE_BUILDER_H
#define EXECUTABLE_FILE_BUILDER_H

#include "Macho.h"

class ExecutableFileBuilder {
    public:
        ExecutableFileBuilder();
        ExecutableFileBuilder(Macho input_macho);

        void buildExecutableFile();
    private:
        Macho input_macho;

        void buildHeader();
};


#endif
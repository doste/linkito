#ifndef EXECUTABLE_FILE_BUILDER2_H
#define EXECUTABLE_FILE_BUILDER2_H

#include "Macho.h"

struct Buffer {
    Byte* data;
    uint64_t size;
    uint64_t capacity;
    uint64_t writing_offset;

    Buffer();
    uint64_t appendData(void* data, uint64_t data_size);
    uint64_t writeDataAtOffset(void* data, uint64_t data_size, uint64_t offset);
};


class ExecutableFileBuilder2 {
    friend class Debugger;

    public:
        ExecutableFileBuilder2();
        ExecutableFileBuilder2(Macho);

        void buildExecutableFile();
        Byte* wholeFile;

        void debug();

    private:
        Macho input_macho;
        Macho output_macho;
        void buildHeader();
        uint64_t writeHeader(uint64_t offset);
        uint64_t header_offset;

        void patchHeader();

        void patchPageZeroSegmentLoadCommand();


        void buildSegments();
        void buildPageZeroSegmentLoadCommand();
        uint64_t writePageZeroSegmentLoadCommand(uint64_t offset);

        SegmentHandle* page_zero_seg_handle;

        Buffer buffer;
        uint64_t appendDataToBuffer(void* data, uint64_t size);
        uint64_t writeDataToBufferAtOffset(void* data, uint64_t data_size, uint64_t offset);
};


#endif
#include "Tester.h"


void Tester::printLoadCommands(Macho macho) {

    std::vector<LoadCommandHandle*> all_handles;

    for (SegmentHandle* segment_handle : macho.segment_handles) {
        all_handles.push_back(segment_handle);
    }
    all_handles.push_back(macho.symtab.symtab_command_handle);
    all_handles.push_back(macho.load_dylinker_handle);
    all_handles.push_back(macho.entry_point_handle);
    all_handles.push_back(macho.uuid_handle);
    all_handles.push_back(macho.source_version_handle);
    all_handles.push_back(macho.load_dylib_handle);

    std::cout << std::endl;
    std::cout << std::endl;
    for (LoadCommandHandle* handle : all_handles) {
        handle->print();
        std::cout << std::endl;
    }

    //for (size_t i = 0; i < macho.header.ncmds; i++) {
    //    std::cout << "Load command " << i << std::endl;
//
    //}
}
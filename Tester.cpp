#include "Tester.h"


void Tester::testLoadCommandsMemoryRegionIsBuiltCorrectly(Macho macho) {
    Byte* load_commands = (Byte*)malloc(sizeof(Byte) * macho.header.sizeofcmds);

    // Obtain the load commands from the file itself:
    FILE* fptr = open_macho_file(macho.file.filename);
    fseek(fptr, sizeof(struct mach_header_64), SEEK_SET);
    size_t items_read = fread(load_commands, macho.header.sizeofcmds, 1, fptr);
    if (items_read != 1) {
        fprintf(stderr, "Error while reading Mach-o load commands.\n");
        exit(1);
    }
    // Compare it with the memory region:
    if (memcmp(load_commands, macho.load_commands_mem_region.region, macho.header.sizeofcmds) == 0) {
        std::cout << "Test passed" << std::endl;
    } else {
        std::cout << "Test failed" << std::endl;
    }
}

void Tester::printLoadCommands(Macho macho) {

    std::vector<LoadCommandHandle*> all_handles;

    // Awfully inefficient but this way we make sure it's the same order as 'otool -l' (first PAGEZERO, then TEXT finally LINKEDIT)
    for (SegmentHandle* segment_handle : *macho.segment_handles) {
        if (segment_handle->segname == SEG_PAGEZERO) {
            all_handles.push_back(segment_handle);
        }
    }
    for (SegmentHandle* segment_handle : *macho.segment_handles) {
        if (segment_handle->segname == SEG_TEXT) {
            all_handles.push_back(segment_handle);
        }
    }
    for (SegmentHandle* segment_handle : *macho.segment_handles) {
        if (segment_handle->segname == SEG_LINKEDIT) {
            all_handles.push_back(segment_handle);
        }
    }

    for (LinkeditDataCommandHandle* linkedit_data_handle : *macho.linkedit_data_handles) {
        if (linkedit_data_handle->load_command->cmd == LC_DYLD_CHAINED_FIXUPS) {
            all_handles.push_back(linkedit_data_handle);
        } 
    }

    for (LinkeditDataCommandHandle* linkedit_data_handle : *macho.linkedit_data_handles) {
        if (linkedit_data_handle->load_command->cmd == LC_DYLD_EXPORTS_TRIE) {
            all_handles.push_back(linkedit_data_handle);
        } 
    }

    all_handles.push_back(macho.symtab.symtab_command_handle);
    all_handles.push_back(macho.dysymtab_handle);
    all_handles.push_back(macho.load_dylinker_handle);
    all_handles.push_back(macho.uuid_handle);
    all_handles.push_back(macho.build_version_handle);
    all_handles.push_back(macho.source_version_handle);
    all_handles.push_back(macho.entry_point_handle);
    all_handles.push_back(macho.load_dylib_handle);
    for (LinkeditDataCommandHandle* linkedit_data_handle : *macho.linkedit_data_handles) {
        if (linkedit_data_handle->load_command->cmd == LC_FUNCTION_STARTS) {
            all_handles.push_back(linkedit_data_handle);
        } 
    }
    for (LinkeditDataCommandHandle* linkedit_data_handle : *macho.linkedit_data_handles) {
        if (linkedit_data_handle->load_command->cmd == LC_DATA_IN_CODE) {
            all_handles.push_back(linkedit_data_handle);
        } 
    }
    for (LinkeditDataCommandHandle* linkedit_data_handle : *macho.linkedit_data_handles) {
        if (linkedit_data_handle->load_command->cmd == LC_CODE_SIGNATURE) {
            all_handles.push_back(linkedit_data_handle);
        } 
    }

    std::cout << std::endl;
    std::cout << std::endl;
    for (size_t i = 0; i < all_handles.size(); i++) {
        LoadCommandHandle* handle = all_handles[i];
        std::cout << "Load command " << i << std::endl;
        handle->print();
    }
    
    //for (LoadCommandHandle* handle : all_handles) {
    //    handle->print();
    //    std::cout << std::endl;
    //}

    //for (size_t i = 0; i < macho.header.ncmds; i++) {
    //    std::cout << "Load command " << i << std::endl;
//
    //}
}
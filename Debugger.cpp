#include "Debugger.h"
#include <unistd.h>



void Debugger::dumpRawDataToFile(void* data, uint32_t offset, uint32_t size, char* filename) {
    FILE* fptr_out = fopen(filename, "wb+");
    if (!fptr_out) {
        fprintf(stderr, "can't open %s: %s\n", filename, strerror(errno));
		exit(1);
	}
    
    size_t written = fwrite((Byte*)data + offset, sizeof(Byte), size, fptr_out);
    if (written < size) {
        fprintf(stderr, "Error while dumping the payload data into the file.\n");
        exit(1);
    }
}

void Debugger::dumpLowerMemoryRegionToFile(ExecutableFileBuilder file_builder) {
    this->dumpRawDataToFile(file_builder.mem_reg_manager->lowerMemoryRegion->data,
        0,
        file_builder.mem_reg_manager->lowerMemoryRegion->offset_and_size.size,
        "ExecutableFileLowerMemoryRegion_DUMP");
}

void Debugger::dumpUpperMemoryRegionToFile(ExecutableFileBuilder file_builder) {
    this->dumpRawDataToFile(file_builder.mem_reg_manager->upperMemoryRegion->data,
        0,
        file_builder.mem_reg_manager->upperMemoryRegion->offset_and_size.size,
        "ExecutableFileUpperMemoryRegion_DUMP");
}

void Debugger::dumpWholeFileToFile(ExecutableFileBuilder file_builder) {
    this->dumpRawDataToFile(file_builder.wholeFile,
        0,
        PAGE_SIZE,
        "ExecutableFileWholeFile_DUMP");
}

void Debugger::dumpLoadCommandsMemoryRegionToFile(Macho macho, uint32_t offset, uint32_t size) {
    this->dumpRawDataToFile(macho.load_commands_mem_region.region, offset, size, "BuildVersion_LoadCommandMemoryRegion_DUMP");
}

void Debugger::dumpWholeLoadCommandsMemoryRegionToFile(Macho macho) {
    this->dumpRawDataToFile(macho.load_commands_mem_region.region, 0, macho.header.sizeofcmds, "WholeLoadCommandMemoryRegion_DUMP");
}

void Debugger::dumpBuildVersionCommandToFile(Macho macho) {
    size_t command_size = macho.build_version_handle->load_command->cmdsize - (sizeof(struct build_tool_version) * macho.build_version_handle->load_command->ntools);
    this->dumpRawDataToFile(macho.build_version_handle->load_command, 0, command_size, "BuildVersion_DUMP");
}

void Debugger::dumpTextSectionToFile(Macho macho) {
    
    for (SegmentHandle* segment : *macho.segment_handles) {
        for (SectionHandle* sect : segment->sections) {
            if (strcmp(sect->section->sectname, SECT_TEXT) == 0) {
                if (sect->payload) {
                    this->dumpRawDataToFile(sect->payload, 0, sect->section->size, "TextSection_DUMP");
                }
            }
        }
    }
}


void Debugger::dumpLinkeditPayloadsToFile(Macho macho) {
    for (LinkeditDataCommandHandle* linkedit_data : *macho.linkedit_data_handles) {
        if (linkedit_data->payload) {
            this->dumpRawDataToFile(linkedit_data->payload, 0, linkedit_data->load_command->datasize, "LinkeditPayloads_DUMP");
        }
    }
}

void Debugger::debugSegmentCommands(Macho macho) {
    std::cout << "Segment commands:" << std::endl;
    for (SegmentHandle* seg : *macho.segment_handles) {
        std::cout << seg->load_command->segname << std::endl;
        if (seg->load_command->nsects != 0) {
            std::cout << "  With sections:" << std::endl;
            for (SectionHandle* sect : seg->sections) {
                std::cout << "      " <<sect->section->sectname << std::endl;
            }
        }
    }
    
}

void Debugger::debugMacho(Macho macho) {
    std::cout << "File name: " << macho.file.filename << std::endl;
    std::cout << "File type: ";
    switch (macho.filetype) {
        case RelocatableObjectFile:
            std::cout << "Relocatable Object" << std::endl;
            break;
        case ExecutableFile:
            std::cout << "Executable" << std::endl;
            break;
        case DynamicLibrary:
            std::cout << "Dynamic Library" << std::endl;
            break;
        default:
            fprintf(stderr, "Debug Error: Not supported filetype\n");
            exit(1);
    }

    std::cout << "File size: " << macho.file.filesize << std::endl;
}

void Debugger::debugSymbolTable(Macho macho) {
    std::cout << "The SymbolTable has " << macho.symtab.entries.size() << " entries. " << std::endl;
    int i = 0;
    for (SymbolTableEntry entry : macho.symtab.entries) {
        std::cout << "Entry "<< i <<" has index_into_string_table: " << entry.index_into_string_table << std::endl;
        i++;
        switch(entry.type) {
            case UNDF: {
                std::cout << "    type: UNDF" << std::endl;   
                break;
            }
            case ABS: {
                std::cout << "    type: ABS" << std::endl;  
                break;
            }
            case SECT: {
                std::cout << "    type: SECT" << std::endl;   
                break;
            }
            case PBUD: {
                std::cout << "    type: PBUD" << std::endl;     
                break;
            }
            case INDR: { 
                std::cout << "    type: INDR" << std::endl;   
                break;
            }
            default:
                fprintf(stderr, "Error. Invalid symbol type: 0x%x\n", entry.type);
                exit(1);
        }
        std::cout << "    section_number: " << entry.section_number << std::endl; 
        std::cout << "    desc: " << entry.desc << std::endl; 
        std::cout << "    value: " << entry.value << std::endl; 
    }
}

void Debugger::debugStringTable(Macho macho) {
    std::cout << "The StringTable has " << macho.symtab.strtab.entries.size() << " entries. " << std::endl;
    int i = 0;
    for (StringTableEntry entry : macho.symtab.strtab.entries) {
        std::cout << "Entry " << i << " has string: " << entry.string << std::endl;
        std::cout << "and index: " << entry.index_into_table << std::endl;
        i++;   
    }
}
    

void debug_map(LoadCommandsRegion lc_region) {

    std::cout << "There are " << lc_region.offsets.size() << " entries." << std::endl;

    for(std::map<std::string, OffsetAndSize>::iterator iter = lc_region.offsets.begin(); iter != lc_region.offsets.end(); ++iter) {
        std::string cmd = iter->first;
        std::cout << "  cmd: " << cmd << std::endl;

        OffsetAndSize offset_and_size = iter->second;
        std::cout << "  offset: " << offset_and_size.offset << std::endl;
        std::cout << "  size: " << offset_and_size.size << std::endl;
        std::cout << "  -------------" << std::endl;
    }
}
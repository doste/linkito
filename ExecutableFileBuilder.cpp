#include "ExecutableFileBuilder.h"
#include <algorithm>

#include "Debugger.h"

MemoryBlock::MemoryBlock() {}

MemoryBlock::MemoryBlock(uint64_t size) {
    this->data = (Byte*)calloc(size, sizeof(Byte));
    this->offset_and_size = OffsetAndSize(0, size);
}

// If data is NULL, we fill the memory block with zeros.
void MemoryBlock::fillMemoryBlock(void* data) {
    if (!data) {
        memset(this->data, 0, this->offset_and_size.size);
    } else {
        memcpy(this->data, data, this->offset_and_size.size);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////

MemoryRegion::MemoryRegion() {
    this->data = (Byte*)malloc(sizeof(Byte) * INITIAL_CAPACITY);
    this->offset_and_size = OffsetAndSize(0, 0);        // Offset and Size are always the same.
    this->capacity = INITIAL_CAPACITY;
    this->blocks = std::vector<MemoryBlock*>();
}

void MemoryRegion::appendMemoryBlock(MemoryBlock* mem_block) {
    if (this->offset_and_size.size + mem_block->offset_and_size.size > this->capacity) {
        this->capacity = std::max(mem_block->offset_and_size.size, (uint64_t)(2 * this->capacity));
        this->data = (Byte*)realloc(this->data, this->capacity);
    }

    memcpy(this->data + this->offset_and_size.offset, mem_block->data, mem_block->offset_and_size.size);

    uint64_t offset_of_this_block_inside_region = this->offset_and_size.offset;
    this->offset_and_size.offset += mem_block->offset_and_size.size;
    this->offset_and_size.size += mem_block->offset_and_size.size;


    // Once a MemoryBlock is appended to the Region, the block now is 'owned' by the Region,
    // so its fields change:
    //  - data is free'd . That data now resides in the Region.
    //  - offset now is with respect to the start of the Region.
    //  - size remains the same.
    mem_block->offset_and_size.offset = offset_of_this_block_inside_region;
    free(mem_block->data);
    mem_block->data = nullptr;

    this->blocks.push_back(mem_block);
}


////////////////////////////////////////////////////////////////////////////////////////////


MemoryRegionManager::MemoryRegionManager() {
    this->upperMemoryRegion = new MemoryRegion();
    this->lowerMemoryRegion = new MemoryRegion();
}

Byte* MemoryRegionManager::mergeRegions() {
    uint64_t sum_of_sizes_of_both_regions = this->lowerMemoryRegion->offset_and_size.size + this->upperMemoryRegion->offset_and_size.size;
    Byte* bigRegion = (Byte*)malloc(sizeof(Byte) * PAGE_SIZE);
    memset(bigRegion, 0, PAGE_SIZE);

    memcpy(bigRegion, this->upperMemoryRegion->data, this->upperMemoryRegion->offset_and_size.size);

    // So the lower region goes right in the end of the file.
    uint64_t offset_for_lower_region = PAGE_SIZE - this->lowerMemoryRegion->offset_and_size.size;
    memcpy(bigRegion + offset_for_lower_region, this->lowerMemoryRegion->data, this->lowerMemoryRegion->offset_and_size.size);


    return bigRegion;
}


void MemoryRegion::debug() {
    std::cout << "      - Size: " << this->offset_and_size.size << std::endl;
    std::cout << "      - Offset: " << this->offset_and_size.offset << std::endl;
    std::cout << "      - Capacity: " << this->capacity << std::endl;
    std::cout << "      - Blocks: " << std::endl;
    int i = 0;
    for (MemoryBlock* block : this->blocks) {
        std::cout << "          - Block " << i << " Size : "<< block->offset_and_size.size << std::endl;
        std::cout << "          - Block " << i << " Offset : "<< block->offset_and_size.offset << std::endl;
        i++;
        std::cout << std::endl;
    }
}

void MemoryRegionManager::debug() {
    std::cout << "  Upper Memory Region: " << std::endl;
    this->upperMemoryRegion->debug();

    std::cout << "  Lower Memory Region: " << std::endl;
    this->lowerMemoryRegion->debug();
}



////////////////////////////////////////////////////////////////////////////////////////////

ExecutableFileBuilder::ExecutableFileBuilder() {}

ExecutableFileBuilder::ExecutableFileBuilder(Macho input_macho) : input_macho(input_macho) {
    this->mem_reg_manager = new MemoryRegionManager();
    this->output_macho = Macho();
}

void ExecutableFileBuilder::debugMemoryRegionManager() {
    std::cout << "Memory Region Manager: " << std::endl;
    this->mem_reg_manager->debug();
}


// For now the output header will be equal to the input header, except for the following fields:
//  - filetype
//  - ncmds
//  - sizeofcmds
//  - flags
void ExecutableFileBuilder::buildHeader() {
    struct mach_header_64 output_header = this->input_macho.header;
    output_header.filetype = MH_EXECUTE;
    // The other fields will be updated later when the whole file is built.

    MemoryBlock* mem_block_header = new MemoryBlock(sizeof(struct mach_header_64));
    mem_block_header->fillMemoryBlock(&output_header);

    this->mem_reg_manager->upperMemoryRegion->appendMemoryBlock(mem_block_header);

    std::cout << "mem_block_header OFFSET: " << mem_block_header->offset_and_size.offset << std::endl;
    std::cout << "mem_block_header SIZE: " << mem_block_header->offset_and_size.size << std::endl;
}





void ExecutableFileBuilder::buildPageZeroSegment() {
    SegmentHandle* output_page_zero_seg_handle = new SegmentHandle();
    output_page_zero_seg_handle->load_command = new SegmentCommand64();

    *output_page_zero_seg_handle->load_command = (SegmentCommand64){ 
                                                    .segname = SEG_PAGEZERO,
                                                    .vmaddr	= 0x0000000000000000,
                                                    .vmsize	= 0x0000000100000000,
                                                    .fileoff = 0,
                                                    .filesize = 0,
                                                    .maxprot = 0,
                                                    .initprot = 0,
                                                    .nsects	= 0,
                                                    .flags = 0 };
    output_page_zero_seg_handle->load_command->cmd = LC_SEGMENT_64;
    output_page_zero_seg_handle->load_command->cmdsize = sizeof(SegmentCommand64);

    output_page_zero_seg_handle->segname = SEG_PAGEZERO;

    MemoryBlock* mem_block_seg_pagezero = new MemoryBlock(sizeof(SegmentCommand64));
    mem_block_seg_pagezero->fillMemoryBlock(output_page_zero_seg_handle->load_command);
    this->mem_reg_manager->upperMemoryRegion->appendMemoryBlock(mem_block_seg_pagezero);


    std::cout << "mem_block_seg_pagezero OFFSET: " << mem_block_seg_pagezero->offset_and_size.offset << std::endl;
    std::cout << "mem_block_seg_pagezero SIZE: " << mem_block_seg_pagezero->offset_and_size.size << std::endl;

    this->output_macho.segment_handles->push_back(output_page_zero_seg_handle);
}

/*
    __TEXT       ___________   <- 0x100000000
                |           |
                |           |
                |           |
     (1 page)   |           |   
                |-- text ---|  <----- text section load addr will be 0x100004000 - the section size
                |    sect   |
    __LINKEDIT  |___________|  <- 0x100004000
*/

uint64_t calculate_load_addr_for_section(uint64_t seg_load_addr, uint64_t seg_size, Section64* sect) {
    return (seg_load_addr + seg_size) - sect->size;
}

uint64_t calculate_file_offset_for_section(uint64_t seg_file_off, uint64_t seg_filesize, Section64* sect) {
    return (seg_file_off + seg_filesize) - sect->size;
}

void ExecutableFileBuilder::buildTextSegment() {
    SegmentHandle* output_text_seg_handle = new SegmentHandle();
    output_text_seg_handle->load_command = new SegmentCommand64();

    SegmentHandle* input_seg_text_handle = this->input_macho.getSegmentHandleForSegmentNamed(SEG_TEXT);
    assert(input_seg_text_handle != nullptr);
    std::vector<SectionHandle*> input_seg_text_sections = input_seg_text_handle->sections;
    uint32_t input_nsects = input_seg_text_sections.size();

    *output_text_seg_handle->load_command = (SegmentCommand64){ 
                                                    .segname = SEG_TEXT,
                                                    .vmaddr	= STANDARD_EXECUTABLE_LOAD_ADDR,
                                                    .vmsize	= PAGE_SIZE,
                                                    .fileoff = 0,
                                                    .filesize = PAGE_SIZE,
                                                    .maxprot = VM_PROT_READ | VM_PROT_EXECUTE,
                                                    .initprot = VM_PROT_READ | VM_PROT_EXECUTE,
                                                    .nsects	= input_nsects,
                                                    .flags = 0 };
    output_text_seg_handle->load_command->cmd = LC_SEGMENT_64;
    output_text_seg_handle->load_command->cmdsize = sizeof(SegmentCommand64);
    output_text_seg_handle->segname = SEG_TEXT;

    MemoryBlock* mem_block_seg_text = new MemoryBlock(sizeof(SegmentCommand64));
    mem_block_seg_text->fillMemoryBlock(output_text_seg_handle->load_command);
    this->mem_reg_manager->upperMemoryRegion->appendMemoryBlock(mem_block_seg_text);

    std::cout << "mem_block_seg_text OFFSET: " << mem_block_seg_text->offset_and_size.offset << std::endl;
    std::cout << "mem_block_seg_text SIZE: " << mem_block_seg_text->offset_and_size.size << std::endl;


    // For now we'll only care about the __text section
    for (SectionHandle* input_sect_handle : input_seg_text_sections) {
        if (strcmp(input_sect_handle->section->sectname, SECT_TEXT) == 0) {
            SectionHandle* output_sect_handle = new SectionHandle();
            // For now, the only fields that differ are addr and offset, so we copy them:
            memcpy(output_sect_handle->section, input_sect_handle->section, sizeof(Section64));
            output_sect_handle->section->addr = calculate_load_addr_for_section(output_text_seg_handle->load_command->vmaddr,
                output_text_seg_handle->load_command->vmsize, output_sect_handle->section);

            output_sect_handle->section->offset = calculate_file_offset_for_section(output_text_seg_handle->load_command->fileoff,
                output_text_seg_handle->load_command->filesize, output_sect_handle->section);
        
            // Add to the Memory Region (the upper one because it's with the load commands).
            MemoryBlock* mem_block_sect_text = new MemoryBlock(sizeof(Section64));
            mem_block_sect_text->fillMemoryBlock(output_sect_handle->section);
            this->mem_reg_manager->upperMemoryRegion->appendMemoryBlock(mem_block_sect_text);

            // Add that output section to our text segment handle.
            output_text_seg_handle->sections.push_back(output_sect_handle);

            // Copy the payload:
            output_sect_handle->payload = (Byte*)malloc(sizeof(Byte) * input_sect_handle->section->size);
            memcpy(output_sect_handle->payload, input_sect_handle->payload, input_sect_handle->section->size);
        
            std::cout << "mem_block_sect_text OFFSET: " << mem_block_sect_text->offset_and_size.offset << std::endl;
            std::cout << "mem_block_sect_text SIZE: " << mem_block_sect_text->offset_and_size.size << std::endl;

        }
    }

    this->output_macho.segment_handles->push_back(output_text_seg_handle);
}

void ExecutableFileBuilder::buildTextSectionPayload() {

    for (SegmentHandle* seg_handle : *this->output_macho.segment_handles) {
        for (SectionHandle* sect_handle : seg_handle->sections) {
            if (strcmp(sect_handle->section->sectname, SECT_TEXT) == 0) {
                MemoryBlock* mem_block_sect_text_payload = new MemoryBlock(sect_handle->section->size);
                mem_block_sect_text_payload->fillMemoryBlock(sect_handle->payload);
                this->mem_reg_manager->lowerMemoryRegion->appendMemoryBlock(mem_block_sect_text_payload);
            }
        }
    }
}
// This function is to build those LoadCommands that have some data that is next after the struct load command, for
// example those who have a 'name' field, the string describing the name is in memory just after the load command.
void ExecutableFileBuilder::buildLoadCommandAppendingDataJustAfterOffset(LoadCommandHandle* load_command_handle,
                     LoadCommand* load_command,
                     uint32_t cmd, uint32_t cmdsize,
                     uint32_t load_command_size_with_no_data, void* data_to_append, uint32_t size_of_data_to_append) {

    // loader.h : "The cmdsize for 32-bit architectures MUST be a multiple of 4 bytes and for 64-bit architectures MUST be a multiple
    //            of 8 bytes (these are forever the maximum alignment of any load commands)"
    uint32_t cmdsize_aligned = cmdsize;
    if (cmdsize_aligned % 8 != 0) {
        cmdsize_aligned = align_to(cmdsize, 8);
    }
    load_command_handle->load_command = load_command;
    load_command->cmd = cmd;
	load_command->cmdsize = cmdsize_aligned;

    // MemoryBlock for the LoadCommand itself.
    MemoryBlock* mem_block_for_load_command = new MemoryBlock(load_command_size_with_no_data); // load_command_size_with_no_data because usually cmdsize includes the size of the data that follows it,
                                                                                               // and we want to add another MemoryBlock for that data.
    mem_block_for_load_command->fillMemoryBlock(load_command);
    this->mem_reg_manager->upperMemoryRegion->appendMemoryBlock(mem_block_for_load_command);

    // MemoryBlock for the data that follows the LoadCommand.
    MemoryBlock* mem_block_for_appended_data = new MemoryBlock(size_of_data_to_append);
    mem_block_for_appended_data->fillMemoryBlock(data_to_append);
    this->mem_reg_manager->upperMemoryRegion->appendMemoryBlock(mem_block_for_appended_data);

    // We need to make sure that the LoadCommand ends up with a size multiple of 8. So, we add as much padding as we need:
    uint32_t final_size_of_load_command = load_command_size_with_no_data + size_of_data_to_append; // The sum of the sizes of the two MemoryBlocks
    if (cmdsize_aligned != final_size_of_load_command) {    // Then we need to add padding
        uint32_t padding = cmdsize_aligned - final_size_of_load_command;
        // Add a MemoryBlock just for the padding.
        MemoryBlock* mem_block_for_padding= new MemoryBlock(padding);
        mem_block_for_padding->fillMemoryBlock(nullptr);            // So it fills it with zeros
        this->mem_reg_manager->upperMemoryRegion->appendMemoryBlock(mem_block_for_padding);
    }
    
}

void ExecutableFileBuilder::buildLoadCommand(LoadCommandHandle* load_command_handle, LoadCommand* load_command, uint32_t cmd, uint32_t cmdsize) {

    // loader.h : "The cmdsize for 32-bit architectures MUST be a multiple of 4 bytes and for 64-bit architectures MUST be a multiple
    //            of 8 bytes (these are forever the maximum alignment of any load commands)"
    uint32_t cmdsize_aligned = cmdsize;
    if (cmdsize_aligned % 8 != 0) {
        cmdsize_aligned = align_to(cmdsize, 8);
    }
    load_command_handle->load_command = load_command;
    load_command->cmd = cmd;
	load_command->cmdsize = cmdsize_aligned;

    MemoryBlock* mem_block_for_load_command = new MemoryBlock(load_command->cmdsize);
    //MemoryBlock* mem_block_for_load_command = new MemoryBlock(sizeof(load_command)); // If there is something that needs to be written in memory after the LoadCommand,
                                                                                     // it must be done by the caller of this function.
                                                                                     // This MemoryBlock is for the LoadCommand only. 
    mem_block_for_load_command->fillMemoryBlock(load_command);
    this->mem_reg_manager->upperMemoryRegion->appendMemoryBlock(mem_block_for_load_command);
}

void ExecutableFileBuilder::buildLinkeditDataCommand(LinkeditDataCommandHandle* handle, LinkeditDataCommand* linkedit_data_load_command, uint32_t cmd) {
    linkedit_data_load_command->dataoff = 0; // ????				   // file offset of data in __LINKEDIT segment 
	linkedit_data_load_command->datasize = 0; // ????				   // file size of data in __LINKEDIT segment
    this->buildLoadCommand(handle, linkedit_data_load_command, cmd, sizeof(LinkeditDataCommand));
    
    this->output_macho.linkedit_data_handles->push_back(handle);
}


void ExecutableFileBuilder::buildDyldChainedFixupsCommand() {
    DyldChainedFixupsCommandHandle* output_dyld_chained_fixups_handle = new DyldChainedFixupsCommandHandle();
    DyldChainedFixupsCommand* dyld_chained_fixups_handle_lc = new DyldChainedFixupsCommand();

    this->buildLinkeditDataCommand(output_dyld_chained_fixups_handle, dyld_chained_fixups_handle_lc, LC_DYLD_CHAINED_FIXUPS); 
}
void ExecutableFileBuilder::buildExportsTrieCommand() {
    ExportsTrieCommandHandle* output_exports_trie_handle = new ExportsTrieCommandHandle();
    ExportsTrieCommand* output_exports_trie_lc = new ExportsTrieCommand();

    this->buildLinkeditDataCommand(output_exports_trie_handle, output_exports_trie_lc, LC_DYLD_EXPORTS_TRIE); 
}
void ExecutableFileBuilder::buildDataInCodeCommand() {
    DataInCodeCommandHandle* data_in_code_handle = new DataInCodeCommandHandle();
    DataInCodeCommand* data_in_code_lc = new DataInCodeCommand();

    this->buildLinkeditDataCommand(data_in_code_handle, data_in_code_lc, LC_DATA_IN_CODE); 
}
void ExecutableFileBuilder::buildFunctionStartCommand() {
    FunctionStartsCommandHandle* function_starts_handle = new FunctionStartsCommandHandle();
    FunctionStartsCommand* function_starts_lc = new FunctionStartsCommand();

    this->buildLinkeditDataCommand(function_starts_handle, function_starts_lc, LC_FUNCTION_STARTS); 
}
void ExecutableFileBuilder::buildCodeSignatureCommand() {
    CodeSignatureCommandHandle* code_signature_handle = new CodeSignatureCommandHandle();
    CodeSignatureCommand* code_signature_lc = new CodeSignatureCommand();

    this->buildLinkeditDataCommand(code_signature_handle, code_signature_lc, LC_CODE_SIGNATURE);  	
}

// Unused (for now at least)
void ExecutableFileBuilder::buildLinkeditDataCommands() {
    this->buildDyldChainedFixupsCommand();
    this->buildExportsTrieCommand();
    this->buildDataInCodeCommand();
    this->buildFunctionStartCommand();
    this->buildCodeSignatureCommand();
}




void ExecutableFileBuilder::buildLinkeditSegment() {
    SegmentHandle* output_linkedit_seg_handle = new SegmentHandle();
    output_linkedit_seg_handle->load_command = new SegmentCommand64();

    SegmentHandle* text_seg_handle = this->output_macho.getSegmentHandleForSegmentNamed(SEG_TEXT);

    *output_linkedit_seg_handle->load_command = (SegmentCommand64){ 
                                            .segname = SEG_LINKEDIT,
                                            .vmaddr	= STANDARD_EXECUTABLE_LOAD_ADDR + text_seg_handle->load_command->vmsize,
                                            .vmsize	= PAGE_SIZE,
                                            .fileoff = text_seg_handle->load_command->filesize, // The LINKEDIT segment follows the TEXT segment.
                                            .filesize = 0,  // To be defined later. When all the linkedit_data_command are built.
                                            .maxprot = VM_PROT_READ,
                                            .initprot = VM_PROT_READ,
                                            .nsects	= 0,
                                            .flags = 0 };
    output_linkedit_seg_handle->load_command->cmd = LC_SEGMENT_64;
    output_linkedit_seg_handle->load_command->cmdsize = sizeof(SegmentCommand64);
    output_linkedit_seg_handle->segname = SEG_LINKEDIT;

    MemoryBlock* mem_block_seg_linkedit = new MemoryBlock(sizeof(SegmentCommand64));
    mem_block_seg_linkedit->fillMemoryBlock(output_linkedit_seg_handle->load_command);
    this->mem_reg_manager->upperMemoryRegion->appendMemoryBlock(mem_block_seg_linkedit);

    std::cout << "mem_block_seg_linkedit OFFSET: " << mem_block_seg_linkedit->offset_and_size.offset << std::endl;
    std::cout << "mem_block_seg_linkedit SIZE: " << mem_block_seg_linkedit->offset_and_size.size << std::endl;

    this->output_macho.segment_handles->push_back(output_linkedit_seg_handle);

    // Finally build each of the load commands that are associated with the LINKEDIT segment:
    //this->buildLinkeditDataCommands();
    // So we respect the order. Better do them separately in the main buildExecutableFile function.
}

/*
 * The uuid load command contains a single 128-bit unique random number that
 * identifies an object produced by the static link editor.
 
TODO: Modify uuid to be really random.
*/
void ExecutableFileBuilder::buildUuidCommand() {
    UuidCommandCommandHandle* uuid_command_handle = new UuidCommandCommandHandle();
    UuidCommand* uuid_lc = new UuidCommand();

    Byte uuid[16] = {0x78, 0x81, 0xEE, 0x18, 0x51, 0x67, 0x40, 0xC5, 0x83, 0x43, 0x55, 0x9B, 0x3C, 0xE0, 0x1B, 0x1C};
	uuid[6] = (uuid[6] & 0b00001111) | 0b01010000;
	uuid[8] = (uuid[8] & 0b00111111) | 0b10000000;
	memcpy(uuid_lc->uuid, uuid, 16 * sizeof(Byte));

    this->buildLoadCommand(uuid_command_handle, uuid_lc, LC_UUID, sizeof(UuidCommand));
    this->output_macho.uuid_handle = uuid_command_handle;
}


void ExecutableFileBuilder::buildBuildVersionCommand() {

    this->output_macho.build_version_handle = new BuildVersionHandle();
    this->output_macho.build_version_handle->load_command = new BuildVersionCommand();
    this->output_macho.build_version_handle->load_command->cmd = LC_BUILD_VERSION;
	this->output_macho.build_version_handle->load_command->cmdsize = sizeof(BuildVersionCommand) + 1 * sizeof(struct build_tool_version);		// sizeof(struct build_version_command) plus ntools * sizeof(struct build_tool_version)
	this->output_macho.build_version_handle->load_command->platform = PLATFORM_MACOS;		// platform 
	this->output_macho.build_version_handle->load_command->minos = (14 << 16);				// X.Y.Z is encoded in nibbles xxxx.yy.zz         	===>    X = 14 Y = 0  ===>  minos = (14 << 16)
	this->output_macho.build_version_handle->load_command->sdk = (14 << 16) | (5 << 8);		// X.Y.Z is encoded in nibbles xxxx.yy.zz 			===>    X = 14 Y = 5  ===>  sdk = (14 << 16) | (5 << 8)
	this->output_macho.build_version_handle->load_command->ntools = 1;						// number of tool entries following this 
	
    struct build_tool_version btv;
	btv.tool = TOOL_LD;
	btv.version = (1053 << 16) | (12 << 8);  	// version 1053.12   =>  X.Y.Z is encoded in nibbles xxxx.yy.zz  => X = 1053 Y = 12
	// As the struct build_tool_version has size multiple of 8, there is no need to add padding.
    this->output_macho.build_version_handle->tool_versions.push_back(btv);

    // Add to the Memory Region (the upper one because it's with the load commands).
    MemoryBlock* mem_block_build_version_lc = new MemoryBlock(sizeof(BuildVersionCommand));
    mem_block_build_version_lc->fillMemoryBlock(this->output_macho.build_version_handle->load_command);
    this->mem_reg_manager->upperMemoryRegion->appendMemoryBlock(mem_block_build_version_lc);
    // After appending it, mem_block_build_version_lc->data will be null. That memory will be now 'owned' by the upperMemoryRegion.
    // To retrieve it, we would need to access to the upperMemoryRegion->data + mem_block_build_version_lc->offset_and_size.offset

    // Finally append the struct build_tool_version:
    MemoryBlock* mem_block_btv = new MemoryBlock(sizeof(struct build_tool_version));
    mem_block_btv->fillMemoryBlock(&btv);
    this->mem_reg_manager->upperMemoryRegion->appendMemoryBlock(mem_block_btv);

}

void ExecutableFileBuilder::buildSourceVersionCommand() {
    SourceVersionCommandHandle* source_version_handle = new SourceVersionCommandHandle();
    SourceVersionCommand* source_version_lc = new SourceVersionCommand();
    source_version_lc->version = 0;
    this->buildLoadCommand(source_version_handle, source_version_lc, LC_SOURCE_VERSION, sizeof(SourceVersionCommand));
    this->output_macho.source_version_handle = source_version_handle;
}

size_t alignStringLengthToSixteen(char* a_string) {
    return align_to(strlen(a_string) + 1, 16);
}

char* allocMemoryForPathnameAligned(char* pathname, size_t pathname_size_aligned) {
    char* pathname_padded_with_zeros = (char*)calloc(pathname_size_aligned, sizeof(char));
    memcpy(pathname_padded_with_zeros, pathname, strlen(pathname));
    return pathname_padded_with_zeros;
}

void ExecutableFileBuilder::buildLoadDylibCommandHandle() {
    LoadDylibCommandHandle* load_dylib_handle = new LoadDylibCommandHandle();
    LoadDylibCommand* load_dylib_lc = new LoadDylibCommand();
       
    size_t lib_system_path_name_size_aligned = alignStringLengthToSixteen(LIB_SYSTEM_PATH_NAME);
    uint32_t cmdsize = sizeof(LoadDylibCommand) + lib_system_path_name_size_aligned;    
    load_dylib_lc->dylib = (struct dylib) {.name.offset = sizeof(LoadDylibCommand), // the string corresponding to the name would be just after this struct,
                                                                                    // so the offset would be the sum of the sizes of the following fields.
                                                                                    // The offset is from the start of the load command structure! 
                                                                                    // So that sum should include the whole struct (this struct dylib + struct LoadDylibCommand)
                                            .timestamp = 2,   		       	// from https://www.unixtimestamp.com/ IDK
                                            .current_version = (1345 << 16 | 120 << 8 | 2),
                                            .compatibility_version = (1 << 16)
    };	// the library identification

    // The LIB_SYSTEM_PATH_NAME needs to appear in the memory after the load command:
    char* lib_system_path_name_padded_with_zeros = allocMemoryForPathnameAligned(LIB_SYSTEM_PATH_NAME, lib_system_path_name_size_aligned);

    this->buildLoadCommandAppendingDataJustAfterOffset(load_dylib_handle, load_dylib_lc,
                                                        LC_LOAD_DYLIB, cmdsize, sizeof(LoadDylibCommand), 
                                                        lib_system_path_name_padded_with_zeros, lib_system_path_name_size_aligned);


    this->output_macho.load_dylib_handle = load_dylib_handle;
}

void ExecutableFileBuilder::buildDyLinkerCommand() {

    LoadDyLinkerCommandHandle* load_dyld_handle = new LoadDyLinkerCommandHandle();
    LoadDyLinkerCommand* load_dyld_lc = new LoadDyLinkerCommand();

    size_t dyld_path_name_size_aligned = alignStringLengthToSixteen(DYLD_PATH_NAME);
    uint32_t cmdsize = sizeof(LoadDyLinkerCommand) + dyld_path_name_size_aligned;   

	load_dyld_lc->name.offset = sizeof(LoadDyLinkerCommand);   // Because cmdsize includes pathname string.

    // The DYLD_PATH_NAME needs to appear in the memory after the load command:
    char* dyld_path_name_padded_with_zeros = allocMemoryForPathnameAligned(DYLD_PATH_NAME, dyld_path_name_size_aligned);

    this->buildLoadCommandAppendingDataJustAfterOffset(load_dyld_handle, load_dyld_lc,
                                                        LC_LOAD_DYLINKER, cmdsize, sizeof(LoadDyLinkerCommand),
                                                        dyld_path_name_padded_with_zeros, dyld_path_name_size_aligned);
    this->output_macho.load_dylinker_handle = load_dyld_handle;
}

void ExecutableFileBuilder::buildEntryPointCommand() {
    EntryPointCommandHandle* entry_point_handle = new EntryPointCommandHandle();
    EntryPointCommand* entry_point_lc = new EntryPointCommand();
    entry_point_lc->entryoff = 0;                                   // // file (__TEXT) offset of main()    TO BE UPDATED!!!
    entry_point_lc->stacksize = 0;
    this->buildLoadCommand(entry_point_handle, entry_point_lc, LC_MAIN, sizeof(EntryPointCommand));
    this->output_macho.entry_point_handle = entry_point_handle;
}

void ExecutableFileBuilder::buildSymbolTable() {
    this->output_macho.symtab = SymbolTable();
    this->output_macho.symtab.symtab_command_handle = new SymTabCommandHandle();
    this->output_macho.symtab.symtab_command_handle->load_command = new SymTabCommand();

    // TO BE UPDATED!!!
    this->output_macho.symtab.symtab_command_handle->load_command->symoff = 0;
    this->output_macho.symtab.symtab_command_handle->load_command->nsyms = 0;
    this->output_macho.symtab.symtab_command_handle->load_command->stroff = 0;
    this->output_macho.symtab.symtab_command_handle->load_command->strsize = 0;

    this->buildLoadCommand(this->output_macho.symtab.symtab_command_handle,
                            this->output_macho.symtab.symtab_command_handle->load_command, LC_SYMTAB,
                            sizeof(SymTabCommand));
}

void ExecutableFileBuilder::buildDySymbolTable() {
    this->output_macho.dysymtab_handle = new DySymTabHandle();
    this->output_macho.dysymtab_handle->load_command = new DySymTabCommand();

    // TO BE UPDATED!!!
	this->output_macho.dysymtab_handle->load_command->ilocalsym = 0;
	this->output_macho.dysymtab_handle->load_command->nlocalsym  = 0;
	this->output_macho.dysymtab_handle->load_command->iextdefsym  = 0;
	this->output_macho.dysymtab_handle->load_command->nextdefsym  = 2;
	this->output_macho.dysymtab_handle->load_command->iundefsym  = 2;
	this->output_macho.dysymtab_handle->load_command->nundefsym  = 0;
	this->output_macho.dysymtab_handle->load_command->tocoff  = 0;
	this->output_macho.dysymtab_handle->load_command->ntoc  = 0;
	this->output_macho.dysymtab_handle->load_command->modtaboff  = 0;
	this->output_macho.dysymtab_handle->load_command->nmodtab  = 0;
	this->output_macho.dysymtab_handle->load_command->extrefsymoff  = 0;
	this->output_macho.dysymtab_handle->load_command->nextrefsyms  = 0;
	this->output_macho.dysymtab_handle->load_command->indirectsymoff  = 0;
	this->output_macho.dysymtab_handle->load_command->nindirectsyms  = 0;
	this->output_macho.dysymtab_handle->load_command->extreloff  = 0;
	this->output_macho.dysymtab_handle->load_command->nextrel  = 0;
	this->output_macho.dysymtab_handle->load_command->locreloff  = 0;
	this->output_macho.dysymtab_handle->load_command->nlocrel  = 0;

    this->buildLoadCommand(this->output_macho.dysymtab_handle,
                           this->output_macho.dysymtab_handle->load_command, LC_DYSYMTAB,
                            sizeof(DySymTabCommand));
}

void ExecutableFileBuilder::buildStringTable() {
    // Actually it's not a load command per se. It's more like a payload of the symbol table.
}

void ExecutableFileBuilder::buildExecutableFile() {    // We need to sort them correctly!!!
    this->buildHeader();
    this->buildPageZeroSegment();
    this->buildTextSegment();
    this->buildLinkeditSegment();
    this->buildTextSectionPayload();

    this->buildDyldChainedFixupsCommand();
    this->buildExportsTrieCommand();
    this->buildSymbolTable();
    this->buildDySymbolTable();
    this->buildDyLinkerCommand();
    this->buildUuidCommand();
    this->buildBuildVersionCommand();
    this->buildSourceVersionCommand();
    this->buildEntryPointCommand();
    this->buildLoadDylibCommandHandle();
    this->buildFunctionStartCommand();
    this->buildDataInCodeCommand();
    this->buildCodeSignatureCommand();
    //this->buildStringTable();

    
    this->wholeFile = this->mem_reg_manager->mergeRegions();
}

void ExecutableFileBuilder::testLoadCommandsMemoryRegionIsBuiltCorrectly() {

    std::cout << "this->input_macho.header.sizeofcmds>>>>>>>>: " << this->input_macho.header.sizeofcmds << std::endl;
    std::cout << "this->output_macho.header.sizeofcmds>>>>>>>: " << this->output_macho.header.sizeofcmds << std::endl;
    std::cout << ">>>>>>>: " << sizeof(Section64) << std::endl;

    /*
    Byte* load_commands = (Byte*)malloc(sizeof(Byte) * this->output_macho.header.sizeofcmds);

    // Obtain the load commands from the file itself:
    FILE* fptr = open_macho_file(this->output_macho.file.filename);
    fseek(fptr, sizeof(struct mach_header_64), SEEK_SET);
    size_t items_read = fread(load_commands, this->output_macho.header.sizeofcmds, 1, fptr);
    if (items_read != 1) {
        fprintf(stderr, "Error while reading Mach-o load commands.\n");
        exit(1);
    }
    // Compare it with the memory region:
    if (memcmp(load_commands, this->mem_reg_manager->upperMemoryRegion, this->output_macho.header.sizeofcmds) == 0) {
        std::cout << "Test passed for Macho file: " << this->output_macho.file.filename << std::endl;
    } else {
        std::cout << "Test failed for Macho file: " << this->output_macho.file.filename << std::endl;
    }

    */
}
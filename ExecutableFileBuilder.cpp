#include "ExecutableFileBuilder.h"
#include <algorithm>

MemoryBlock::MemoryBlock() {}

MemoryBlock::MemoryBlock(uint64_t size) {
    this->data = (Byte*)malloc(sizeof(Byte) * size);
    this->offset_and_size = OffsetAndSize(0, size);
}

void MemoryBlock::fillMemoryBlock(void* data) {
    assert(data != nullptr);
    memcpy(this->data, data, this->offset_and_size.size);
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

void ExecutableFileBuilder::buildLoadCommand(LoadCommandHandle* load_command_handle, LoadCommand* load_command, uint32_t cmd) {

    load_command_handle->load_command = load_command;
    load_command->cmd = cmd;
	load_command->cmdsize = sizeof(load_command);

    MemoryBlock* mem_block_for_load_command = new MemoryBlock(sizeof(load_command));
    mem_block_for_load_command->fillMemoryBlock(load_command);
    this->mem_reg_manager->upperMemoryRegion->appendMemoryBlock(mem_block_for_load_command);

    //std::cout << "mem_block_for_linkedit_data_lc cmd: " << macroToString[cmd] << " OFFSET: " << mem_block_for_linkedit_data_lc->offset_and_size.offset << std::endl;
    //std::cout << "mem_block_for_linkedit_data_lc cmd: " << macroToString[cmd] << " SIZE: " << mem_block_for_linkedit_data_lc->offset_and_size.size << std::endl;

}

void ExecutableFileBuilder::buildLinkeditDataCommand(LinkeditDataCommandHandle* handle, LinkeditDataCommand* linkedit_data_load_command, uint32_t cmd) {
    linkedit_data_load_command->dataoff = 0; // ????				   // file offset of data in __LINKEDIT segment 
	linkedit_data_load_command->datasize = 0; // ????				   // file size of data in __LINKEDIT segment
    this->buildLoadCommand(handle, linkedit_data_load_command, cmd);
    
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
    this->buildLinkeditDataCommands();
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

    this->buildLoadCommand(uuid_command_handle, uuid_lc, LC_UUID);
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
    this->buildLoadCommand(source_version_handle, source_version_lc, LC_SOURCE_VERSION);
    this->output_macho.source_version_handle = source_version_handle;
}
void ExecutableFileBuilder::buildLoadDylibCommandHandle() {
    
}

void ExecutableFileBuilder::buildDyLinkerCommand() {

}
void ExecutableFileBuilder::buildEntryPointCommand() {

}
void ExecutableFileBuilder::buildSymbolTable() {

}
void ExecutableFileBuilder::buildDySymbolTable() {

}
void ExecutableFileBuilder::buildStringTable() {

}

void ExecutableFileBuilder::buildExecutableFile() {    // We need to sort them correctly!!!
    this->buildHeader();
    this->buildPageZeroSegment();
    this->buildTextSegment();
    this->buildLinkeditSegment();
    this->buildTextSectionPayload();

    this->buildBuildVersionCommand();

    this->buildSymbolTable();
    this->buildDySymbolTable();
    this->buildStringTable();

    this->buildDyLinkerCommand();
    this->buildEntryPointCommand();
    this->buildUuidCommand();
    this->buildSourceVersionCommand();
    this->buildLoadDylibCommandHandle();


    this->wholeFile = this->mem_reg_manager->mergeRegions();
}
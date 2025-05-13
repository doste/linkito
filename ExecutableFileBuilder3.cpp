#include "ExecutableFileBuilder3.h"


#include "Debugger.h"

/////////////////////////////////// MemoryBlock / MemoryRegion / LoadCommandsRegion ////////////////////////////////


MemoryBlock::MemoryBlock() {}

MemoryBlock::MemoryBlock(OffsetAndSize of_sz) {
    this->data = (Byte*)calloc(of_sz.size, sizeof(Byte));
    this->offset_and_size = of_sz;
}

MemoryBlock::MemoryBlock(Byte* data, OffsetAndSize of_sz) {
    this->data = (Byte*)calloc(of_sz.size, sizeof(Byte));
    this->offset_and_size = of_sz;
    memcpy(this->data, data, this->offset_and_size.size);
}

void MemoryBlock::fillWithZeros() {
    memset(this->data, 0, this->offset_and_size.size);
}


MemoryRegion::MemoryRegion() {
    this->data = (Byte*)calloc(sizeof(Byte), INITIAL_CAPACITY);
    this->offset_and_size = OffsetAndSize(0, 0);        // Offset and Size are always the same.
    this->capacity = INITIAL_CAPACITY;
    this->blocks = std::vector<MemoryBlock>();
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
}

LoadCommandsRegion2::LoadCommandsRegion2() {
    this->region = MemoryRegion();
	this->offsets = std::map<std::string, OffsetAndSize>();
}

void LoadCommandsRegion2::appendHeader(MachHeader64* header) {
    auto mem_block_for_header = new MemoryBlock(reinterpret_cast<Byte*>(header), OffsetAndSize(0, sizeof(MachHeader64)));
    this->region.appendMemoryBlock(mem_block_for_header);
}

std::string buildKeyForLoadCommand(LoadCommand* lc) {
    std::string key_cmd_name = std::string{macroToString[lc->cmd]};
    OffsetAndSize offset_and_size;
    if (lc->cmd == LC_SEGMENT_64) { // If it's a Segment, we need to concatenate its segname to the key string
        SegmentCommand64* seg_cmd = static_cast<SegmentCommand64*>(lc);
        key_cmd_name = key_cmd_name + ":";
        key_cmd_name = key_cmd_name + std::string{seg_cmd->segname};
    }
    return key_cmd_name;
}

// Just concatenate the section's sectname and segname fields.
std::string buildKeyForSectionCommand(Section64* sect_cmd) {
    std::string key_cmd_name = sect_cmd->sectname;
    key_cmd_name += ":";
    key_cmd_name += sect_cmd->segname;
    return key_cmd_name;
}

void LoadCommandsRegion2::appendSectionCommand(Section64* sect_cmd) {
    auto mem_block_for_sect_cmd = new MemoryBlock(reinterpret_cast<Byte*>(sect_cmd), 
                                                                OffsetAndSize(0, sizeof(Section64)));
    this->region.appendMemoryBlock(mem_block_for_sect_cmd);
    // Once mem_block_for_lc is appended to the region, its offset given by '->offset_and_size.offset' will be
    // with respect to the whole region. So we use that as offset for the 'offsets' dictionary entry.

    std::string key_cmd_name = buildKeyForSectionCommand(sect_cmd);

    auto offset_and_size = OffsetAndSize(mem_block_for_sect_cmd->offset_and_size.offset,
                                        mem_block_for_sect_cmd->offset_and_size.size);

    this->offsets.insert(std::make_pair(key_cmd_name, offset_and_size));
}

void LoadCommandsRegion2::appendLoadCommand(LoadCommand* lc, uint64_t size_of_lc) {
    auto mem_block_for_lc = new MemoryBlock(reinterpret_cast<Byte*>(lc), 
                                                                OffsetAndSize(0, size_of_lc));
    this->region.appendMemoryBlock(mem_block_for_lc);
    // Once mem_block_for_lc is appended to the region, its offset given by '->offset_and_size.offset' will be
    // with respect to the whole region. So we use that as offset for the 'offsets' dictionary entry.

    std::string key_cmd_name = buildKeyForLoadCommand(lc);
    
    auto offset_and_size = OffsetAndSize(mem_block_for_lc->offset_and_size.offset,
                                         mem_block_for_lc->offset_and_size.size);

    this->offsets.insert(std::make_pair(key_cmd_name, offset_and_size));
}

// This function assumes size_of_data is multiple of 8 data_to_append is already padded with zeros.
void LoadCommandsRegion2::appendLoadCommandWithDataAfterIt(LoadCommand* lc, uint64_t cmdsize_no_data, 
                                                            Byte* data_to_append, uint64_t size_of_data) {                                              
    // First the load command itself:
    this->appendLoadCommand(lc, cmdsize_no_data);

    // And then the data after it:                                                            
    auto mem_block_for_appended_data = new MemoryBlock(data_to_append, OffsetAndSize(0, size_of_data));
    this->region.appendMemoryBlock(mem_block_for_appended_data);

    // loader.h : "The cmdsize for 64-bit architectures MUST be a multiple of 8 bytes"
    // So we add padding as needed:
    assert((cmdsize_no_data + size_of_data) <= lc->cmdsize);
    uint64_t padding_size = lc->cmdsize - (cmdsize_no_data + size_of_data);
    if (padding_size > 0) {
        //std::cout << "PADDING: " << padding_size << std::endl;
        auto padding_mem_block = new MemoryBlock(OffsetAndSize(0, padding_size));
        padding_mem_block->fillWithZeros();
        this->region.appendMemoryBlock(padding_mem_block);
    }

    // The call to this->appendLoadCommand() in the first line up there, already appended this load command,
    // so, now we need to update the load command 'this->offsets's entry, so now its size takes into account the data appended:
    std::string key_cmd_name = buildKeyForLoadCommand(lc);
    uint64_t new_offset = this->offsets[key_cmd_name].offset;                               // Offset doesn't change
    uint64_t new_size = this->offsets[key_cmd_name].size + size_of_data + padding_size;     // Size does!
    this->offsets[key_cmd_name] = OffsetAndSize(new_offset, new_size);
}


MemoryRegionManager::MemoryRegionManager() {
    this->loadCommandsMemoryRegion = new LoadCommandsRegion2();
    this->lowerMemoryRegion = new MemoryRegion();
}

Byte* MemoryRegionManager::mergeRegions() {
    //uint64_t sum_of_sizes_of_both_regions = this->lowerMemoryRegion->offset_and_size.size + this->upperMemoryRegion->offset_and_size.size;
    Byte* bigRegion = (Byte*)malloc(sizeof(Byte) * PAGE_SIZE);
    memset(bigRegion, 0, PAGE_SIZE);

    //memcpy(bigRegion, this->upperMemoryRegion->data, this->upperMemoryRegion->offset_and_size.size);

    // So the lower region goes right in the end of the file.
    uint64_t offset_for_lower_region = PAGE_SIZE - this->lowerMemoryRegion->offset_and_size.size;
    memcpy(bigRegion + offset_for_lower_region, this->lowerMemoryRegion->data, this->lowerMemoryRegion->offset_and_size.size);


    return bigRegion;
}


/////////////////////////////////// ExecutableFileBuilder ////////////////////////////////

ExecutableFileBuilder::ExecutableFileBuilder() {}

ExecutableFileBuilder::ExecutableFileBuilder(Macho input_macho) : input_macho(input_macho) {
    this->mem_reg_manager = new MemoryRegionManager();
    this->output_macho = Macho();
}

/////////////////////////////////// ExecutableFileBuilder : Build's ////////////////////////////////

void ExecutableFileBuilder::buildExecutableFile() { 

    this->buildHeader();
    this->buildPageZeroSegment();
    this->buildTextSegment();
    this->buildLinkeditSegment();
    //this->buildTextSectionPayload();

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

    // Just to test the patching:
    this->patchPageZeroSegment();
    this->patchLinkeditDataCommand(LC_DYLD_CHAINED_FIXUPS, 0xDDDD, 0xCCCC);
    this->patchEntryPointCommand(0xAA);


   this->debugOffsetsMap();
}

void ExecutableFileBuilder::buildHeader() {

    this->output_macho.header->magic = MH_MAGIC_64;
    this->output_macho.header->cputype = CPU_TYPE_ARM64;	        
    this->output_macho.header->cpusubtype = 0;	       
    this->output_macho.header->filetype = MH_CORE;	    // <- CHANGEEEEEEE just to test it for now      
    this->output_macho.header->ncmds = 0;		     // TO UPDATE!     
    this->output_macho.header->sizeofcmds = 0;	     // TO UPDATE! 
    this->output_macho.header->flags = MH_NOUNDEFS | MH_DYLDLINK | MH_TWOLEVEL | MH_PIE;      
    
    
    // The other fields will be updated later when the whole file is built.

    this->mem_reg_manager->loadCommandsMemoryRegion->appendHeader(this->output_macho.header);
}


void ExecutableFileBuilder::buildPageZeroSegment() {
    SegmentHandle* output_page_zero_seg_handle = new SegmentHandle();

    output_page_zero_seg_handle->segname = SEG_PAGEZERO;
    strcpy(static_cast<SegmentCommand64*>(output_page_zero_seg_handle->load_command)->segname, SEG_PAGEZERO);
    static_cast<SegmentCommand64*>(output_page_zero_seg_handle->load_command)->vmaddr	= 0x0000000000000000;
    static_cast<SegmentCommand64*>(output_page_zero_seg_handle->load_command)->vmsize	= 0x0000000100000000;
    static_cast<SegmentCommand64*>(output_page_zero_seg_handle->load_command)->fileoff = 0;
    static_cast<SegmentCommand64*>(output_page_zero_seg_handle->load_command)->filesize = 0;
    static_cast<SegmentCommand64*>(output_page_zero_seg_handle->load_command)->maxprot = 0;
    static_cast<SegmentCommand64*>(output_page_zero_seg_handle->load_command)->initprot = 0;
    static_cast<SegmentCommand64*>(output_page_zero_seg_handle->load_command)->nsects	= 0;
    static_cast<SegmentCommand64*>(output_page_zero_seg_handle->load_command)->flags = 0 ;

    this->mem_reg_manager->loadCommandsMemoryRegion->appendLoadCommand(output_page_zero_seg_handle->load_command, 
                                                                        sizeof(SegmentCommand64));
    
    this->output_macho.segment_handles->push_back(output_page_zero_seg_handle);
}

///////////// Text Segment

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

// TODO: Improve this function. It's a mess.
void ExecutableFileBuilder::buildTextSegment() {
    SegmentHandle* output_text_seg_handle = new SegmentHandle();

    SegmentHandle* input_seg_text_handle = this->input_macho.getSegmentHandleForSegmentNamed(SEG_TEXT);
    assert(input_seg_text_handle != nullptr);
    std::vector<SectionHandle*> input_seg_text_sections = input_seg_text_handle->sections;
    //uint32_t input_nsects = input_seg_text_sections.size(); // For now we'll only care about the __text section
    uint32_t nsects = 1;
           
    output_text_seg_handle->segname = SEG_TEXT;
    strcpy(static_cast<SegmentCommand64*>(output_text_seg_handle->load_command)->segname, SEG_TEXT);
    static_cast<SegmentCommand64*>(output_text_seg_handle->load_command)->vmaddr = STANDARD_EXECUTABLE_LOAD_ADDR;
    static_cast<SegmentCommand64*>(output_text_seg_handle->load_command)->vmsize = PAGE_SIZE;
    static_cast<SegmentCommand64*>(output_text_seg_handle->load_command)->fileoff = 0;
    static_cast<SegmentCommand64*>(output_text_seg_handle->load_command)->filesize = PAGE_SIZE;
    static_cast<SegmentCommand64*>(output_text_seg_handle->load_command)->maxprot = VM_PROT_READ | VM_PROT_EXECUTE;
    static_cast<SegmentCommand64*>(output_text_seg_handle->load_command)->initprot = VM_PROT_READ | VM_PROT_EXECUTE;
    static_cast<SegmentCommand64*>(output_text_seg_handle->load_command)->nsects = nsects;
    static_cast<SegmentCommand64*>(output_text_seg_handle->load_command)->flags = 0;
    static_cast<SegmentCommand64*>(output_text_seg_handle->load_command)->cmdsize = sizeof(SegmentCommand64) + (nsects * sizeof(Section64));

    this->mem_reg_manager->loadCommandsMemoryRegion->appendLoadCommand(output_text_seg_handle->load_command, 
        sizeof(SegmentCommand64));

    // For now we'll only care about the __text section
    for (SectionHandle* input_sect_handle : input_seg_text_sections) {
        if (strcmp(input_sect_handle->section->sectname, SECT_TEXT) == 0) {
            SectionHandle* output_sect_handle = new SectionHandle();
            // For now, the only fields that differ are addr and offset, so we copy them:
            memcpy(output_sect_handle->section, input_sect_handle->section, sizeof(Section64));
                output_sect_handle->section->addr = calculate_load_addr_for_section(static_cast<SegmentCommand64*>(output_text_seg_handle->load_command)->vmaddr,
                static_cast<SegmentCommand64*>(output_text_seg_handle->load_command)->vmsize, output_sect_handle->section);

            output_sect_handle->section->offset = calculate_file_offset_for_section(static_cast<SegmentCommand64*>(output_text_seg_handle->load_command)->fileoff,
                static_cast<SegmentCommand64*>(output_text_seg_handle->load_command)->filesize, output_sect_handle->section);
        
            // Add to the Memory Region (the upper one because it's with the load commands).
            this->mem_reg_manager->loadCommandsMemoryRegion->appendSectionCommand(output_sect_handle->section);

            // Add that output section to our text segment handle.
            output_text_seg_handle->sections.push_back(output_sect_handle);

            // Copy the payload:
            output_sect_handle->payload = (Byte*)malloc(sizeof(Byte) * input_sect_handle->section->size);
            memcpy(output_sect_handle->payload, input_sect_handle->payload, input_sect_handle->section->size);
        }
    }

    this->output_macho.segment_handles->push_back(output_text_seg_handle);
}


///////////// Linkedit Segment

void ExecutableFileBuilder::buildLinkeditSegment() {
    SegmentHandle* output_linkedit_seg_handle = new SegmentHandle();

    SegmentHandle* text_seg_handle = this->output_macho.getSegmentHandleForSegmentNamed(SEG_TEXT);

    uint64_t fileoff = static_cast<SegmentCommand64*>(text_seg_handle->load_command)->filesize;
    uint64_t vmaddr = STANDARD_EXECUTABLE_LOAD_ADDR + static_cast<SegmentCommand64*>(text_seg_handle->load_command)->vmsize;

    output_linkedit_seg_handle->segname = SEG_LINKEDIT;
    strcpy(static_cast<SegmentCommand64*>(output_linkedit_seg_handle->load_command)->segname, SEG_LINKEDIT);
    static_cast<SegmentCommand64*>(output_linkedit_seg_handle->load_command)->vmaddr = vmaddr;
    static_cast<SegmentCommand64*>(output_linkedit_seg_handle->load_command)->vmsize = PAGE_SIZE;
    static_cast<SegmentCommand64*>(output_linkedit_seg_handle->load_command)->fileoff = fileoff; // The LINKEDIT segment follows the TEXT segment.
    static_cast<SegmentCommand64*>(output_linkedit_seg_handle->load_command)->filesize = 0;      // To be defined later. When all the linkedit_data_command are built.
    static_cast<SegmentCommand64*>(output_linkedit_seg_handle->load_command)->maxprot = VM_PROT_READ;
    static_cast<SegmentCommand64*>(output_linkedit_seg_handle->load_command)->initprot = VM_PROT_READ;
    static_cast<SegmentCommand64*>(output_linkedit_seg_handle->load_command)->nsects = 0;
    static_cast<SegmentCommand64*>(output_linkedit_seg_handle->load_command)->flags = 0;

    this->mem_reg_manager->loadCommandsMemoryRegion->appendLoadCommand(output_linkedit_seg_handle->load_command, 
        sizeof(SegmentCommand64));

    this->output_macho.segment_handles->push_back(output_linkedit_seg_handle);
}

void ExecutableFileBuilder::buildLoadCommandAppendingDataJustAfter(LoadCommandHandle* load_command_handle,
                                                                    uint32_t cmdsize_no_data,
                                                                    Byte* data_to_append,
                                                                    uint32_t size_of_data_to_append) {
    assert(load_command_handle);
    assert(load_command_handle->load_command);

    // loader.h : "The cmdsize for 64-bit architectures MUST be a multiple of 8 bytes"   
    if (load_command_handle->load_command->cmdsize % 8 != 0) {
        load_command_handle->load_command->cmdsize = align_to(load_command_handle->load_command->cmdsize, 8);
    }

    this->mem_reg_manager->loadCommandsMemoryRegion->appendLoadCommandWithDataAfterIt(load_command_handle->load_command,
                        cmdsize_no_data,
                        data_to_append, size_of_data_to_append);
}

// TODO: Do we really need cmdsize?
void ExecutableFileBuilder::buildLoadCommand(LoadCommandHandle* load_command_handle, uint32_t cmdsize) {
    assert(load_command_handle);
    assert(load_command_handle->load_command);

    // loader.h : "The cmdsize for 32-bit architectures MUST be a multiple of 4 bytes and for 64-bit architectures MUST be a multiple
    //            of 8 bytes (these are forever the maximum alignment of any load commands)"
    uint32_t cmdsize_aligned = cmdsize;
    if (cmdsize_aligned % 8 != 0) {
        cmdsize_aligned = align_to(cmdsize, 8);
    }
    load_command_handle->load_command->cmdsize = cmdsize_aligned;

    this->mem_reg_manager->loadCommandsMemoryRegion->appendLoadCommand(load_command_handle->load_command, load_command_handle->load_command->cmdsize);  
}

void ExecutableFileBuilder::buildLinkeditDataCommand(LinkeditDataCommandHandle* handle) {
    //linkedit_data_load_command->dataoff = 0; // ????				   // file offset of data in __LINKEDIT segment 
	//linkedit_data_load_command->datasize = 0; // ????				   // file size of data in __LINKEDIT segment
    assert(handle);
    assert(handle->load_command);

    this->mem_reg_manager->loadCommandsMemoryRegion->appendLoadCommand(handle->load_command, handle->load_command->cmdsize);
    this->output_macho.linkedit_data_handles->push_back(handle);
}


void ExecutableFileBuilder::buildDyldChainedFixupsCommand() {
    DyldChainedFixupsCommandHandle* output_dyld_chained_fixups_handle = new DyldChainedFixupsCommandHandle();

    this->buildLinkeditDataCommand(output_dyld_chained_fixups_handle); 
}
void ExecutableFileBuilder::buildExportsTrieCommand() {
    ExportsTrieCommandHandle* output_exports_trie_handle = new ExportsTrieCommandHandle();

    this->buildLinkeditDataCommand(output_exports_trie_handle); 
}
void ExecutableFileBuilder::buildDataInCodeCommand() {
    DataInCodeCommandHandle* data_in_code_handle = new DataInCodeCommandHandle();

    this->buildLinkeditDataCommand(data_in_code_handle); 
}
void ExecutableFileBuilder::buildFunctionStartCommand() {
    FunctionStartsCommandHandle* function_starts_handle = new FunctionStartsCommandHandle();

    this->buildLinkeditDataCommand(function_starts_handle); 
}
void ExecutableFileBuilder::buildCodeSignatureCommand() {
    CodeSignatureCommandHandle* code_signature_handle = new CodeSignatureCommandHandle();

    this->buildLinkeditDataCommand(code_signature_handle);  	
}

void ExecutableFileBuilder::buildSymbolTable() {
    SymbolTable symtab = SymbolTable();
    symtab.symtab_command_handle = new SymTabCommandHandle();

    // TO BE UPDATED!!!
    static_cast<SymTabCommand*>(this->output_macho.symtab.symtab_command_handle->load_command)->symoff = 0;
    static_cast<SymTabCommand*>(this->output_macho.symtab.symtab_command_handle->load_command)->nsyms = 0;
    static_cast<SymTabCommand*>(this->output_macho.symtab.symtab_command_handle->load_command)->stroff = 0;
    static_cast<SymTabCommand*>(this->output_macho.symtab.symtab_command_handle->load_command)->strsize = 0;

    this->buildLoadCommand(symtab.symtab_command_handle, sizeof(SymTabCommand));
    this->output_macho.symtab = symtab;
}
void ExecutableFileBuilder::buildDySymbolTable() {
    DySymTabHandle* dysymtab_handle = new DySymTabHandle();

    // TO BE UPDATED!!!
	static_cast<DySymTabCommand*>(dysymtab_handle->load_command)->ilocalsym = 0;
	static_cast<DySymTabCommand*>(dysymtab_handle->load_command)->nlocalsym  = 0;
	static_cast<DySymTabCommand*>(dysymtab_handle->load_command)->iextdefsym  = 0;
	static_cast<DySymTabCommand*>(dysymtab_handle->load_command)->nextdefsym  = 2;
	static_cast<DySymTabCommand*>(dysymtab_handle->load_command)->iundefsym  = 2;
	static_cast<DySymTabCommand*>(dysymtab_handle->load_command)->nundefsym  = 0;
	static_cast<DySymTabCommand*>(dysymtab_handle->load_command)->tocoff  = 0;
	static_cast<DySymTabCommand*>(dysymtab_handle->load_command)->ntoc  = 0;
	static_cast<DySymTabCommand*>(dysymtab_handle->load_command)->modtaboff  = 0;
	static_cast<DySymTabCommand*>(dysymtab_handle->load_command)->nmodtab  = 0;
	static_cast<DySymTabCommand*>(dysymtab_handle->load_command)->extrefsymoff  = 0;
	static_cast<DySymTabCommand*>(dysymtab_handle->load_command)->nextrefsyms  = 0;
	static_cast<DySymTabCommand*>(dysymtab_handle->load_command)->indirectsymoff  = 0;
	static_cast<DySymTabCommand*>(dysymtab_handle->load_command)->nindirectsyms  = 0;
	static_cast<DySymTabCommand*>(dysymtab_handle->load_command)->extreloff  = 0;
	static_cast<DySymTabCommand*>(dysymtab_handle->load_command)->nextrel  = 0;
	static_cast<DySymTabCommand*>(dysymtab_handle->load_command)->locreloff  = 0;
	static_cast<DySymTabCommand*>(dysymtab_handle->load_command)->nlocrel  = 0;

    this->buildLoadCommand(dysymtab_handle, sizeof(DySymTabCommand));
    this->output_macho.dysymtab_handle = dysymtab_handle;
}

// cmdsize includes pathname string.
void ExecutableFileBuilder::buildDyLinkerCommand() {
    LoadDyLinkerCommandHandle* load_dyld_handle = new LoadDyLinkerCommandHandle();

    size_t dyld_path_name_size_aligned = alignStringLengthToSixteen(DYLD_PATH_NAME);

	static_cast<LoadDyLinkerCommand*>(load_dyld_handle->load_command)->name.offset = sizeof(LoadDyLinkerCommand);

    // The DYLD_PATH_NAME needs to appear in the memory after the load command:
    char* dyld_path_name_padded_with_zeros = allocMemoryForPathnameAligned(DYLD_PATH_NAME, dyld_path_name_size_aligned);

    this->buildLoadCommandAppendingDataJustAfter(load_dyld_handle, sizeof(LoadDyLinkerCommand),
        reinterpret_cast<Byte*>(dyld_path_name_padded_with_zeros), dyld_path_name_size_aligned);

    this->output_macho.load_dylinker_handle = load_dyld_handle;
}

/*
 * The uuid load command contains a single 128-bit unique random number that
 * identifies an object produced by the static link editor.
 
TODO: Modify uuid to be really random.
*/
void ExecutableFileBuilder::buildUuidCommand() {
    UuidCommandHandle* uuid_command_handle = new UuidCommandHandle();

    Byte uuid[16] = {0x78, 0x81, 0xEE, 0x18, 0x51, 0x67, 0x40, 0xC5, 0x83, 0x43, 0x55, 0x9B, 0x3C, 0xE0, 0x1B, 0x1C};
	uuid[6] = (uuid[6] & 0b00001111) | 0b01010000;
	uuid[8] = (uuid[8] & 0b00111111) | 0b10000000;
	memcpy(static_cast<UuidCommand*>(uuid_command_handle->load_command)->uuid, uuid, 16 * sizeof(Byte));

    this->buildLoadCommand(uuid_command_handle, sizeof(UuidCommand));
    this->output_macho.uuid_handle = uuid_command_handle;
}

void ExecutableFileBuilder::buildBuildVersionCommand() {
    BuildVersionHandle* build_version_handle = new BuildVersionHandle();
    uint32_t ntools = 1;
	static_cast<BuildVersionCommand*>(build_version_handle->load_command)->cmdsize = sizeof(BuildVersionCommand) + ntools * sizeof(struct build_tool_version);	// sizeof(struct build_version_command) plus ntools * sizeof(struct build_tool_version)
	static_cast<BuildVersionCommand*>(build_version_handle->load_command)->platform = PLATFORM_MACOS;		// platform 
	static_cast<BuildVersionCommand*>(build_version_handle->load_command)->minos = (14 << 16);				// X.Y.Z is encoded in nibbles xxxx.yy.zz         	===>    X = 14 Y = 0  ===>  minos = (14 << 16)
	static_cast<BuildVersionCommand*>(build_version_handle->load_command)->sdk = (14 << 16) | (5 << 8);		// X.Y.Z is encoded in nibbles xxxx.yy.zz 			===>    X = 14 Y = 5  ===>  sdk = (14 << 16) | (5 << 8)
	static_cast<BuildVersionCommand*>(build_version_handle->load_command)->ntools = ntools;					// number of tool entries following this 
	
    struct build_tool_version btv;
	btv.tool = TOOL_LD;
	btv.version = (1053 << 16) | (12 << 8);  	// version 1053.12   =>  X.Y.Z is encoded in nibbles xxxx.yy.zz  => X = 1053 Y = 12
	// As the struct build_tool_version has size multiple of 8, there is no need to add padding.
    build_version_handle->tool_versions.push_back(btv);

    this->buildLoadCommandAppendingDataJustAfter(build_version_handle, sizeof(BuildVersionCommand),
                reinterpret_cast<Byte*>(&btv), sizeof(struct build_tool_version));

    this->output_macho.build_version_handle = build_version_handle;
}


void ExecutableFileBuilder::buildSourceVersionCommand() {
    SourceVersionCommandHandle* source_version_handle = new SourceVersionCommandHandle();
    static_cast<SourceVersionCommand*>(source_version_handle->load_command)->version = 0;
    this->buildLoadCommand(source_version_handle, sizeof(SourceVersionCommand));
    this->output_macho.source_version_handle = source_version_handle;
}


void ExecutableFileBuilder::buildEntryPointCommand() {
    EntryPointCommandHandle* entry_point_handle = new EntryPointCommandHandle();
    static_cast<EntryPointCommand*>(entry_point_handle->load_command)->entryoff = 0;  // file (__TEXT) offset of main()   TO BE UPDATED!!!
    static_cast<EntryPointCommand*>(entry_point_handle->load_command)->stacksize = 0;

    this->buildLoadCommand(entry_point_handle, sizeof(EntryPointCommand));
    this->output_macho.entry_point_handle = entry_point_handle;
}


void ExecutableFileBuilder::buildLoadDylibCommandHandle() {
    LoadDylibCommandHandle* load_dylib_handle = new LoadDylibCommandHandle();
        
    static_cast<LoadDylibCommand*>(load_dylib_handle->load_command)->dylib = (struct dylib) {.name.offset = sizeof(LoadDylibCommand), // the string corresponding to the name would be just after this struct,
                                                                                    // so the offset would be the sum of the sizes of the following fields.
                                                                                    // The offset is from the start of the load command structure! 
                                                                                    // So that sum should include the whole struct (this struct dylib + struct LoadDylibCommand)
                                                                                .timestamp = 2,   		       	// from https://www.unixtimestamp.com/ IDK
                                                                                .current_version = (1345 << 16 | 120 << 8 | 2),
                                                                                .compatibility_version = (1 << 16)
                                                                                };	// the library identification
    
    uint64_t lib_system_path_name_size_aligned = alignStringLengthToSixteen(LIB_SYSTEM_PATH_NAME);                                                                       
    // The LIB_SYSTEM_PATH_NAME needs to appear in the memory after the load command:
    char* lib_system_path_name_padded_with_zeros = allocMemoryForPathnameAligned(LIB_SYSTEM_PATH_NAME, lib_system_path_name_size_aligned);

    this->buildLoadCommandAppendingDataJustAfter(load_dylib_handle, sizeof(LoadDylibCommand),
        reinterpret_cast<Byte*>(lib_system_path_name_padded_with_zeros), lib_system_path_name_size_aligned);                                                    

    this->output_macho.load_dylib_handle = load_dylib_handle;
}

/////////////////////////////////// ExecutableFileBuilder : Patching's ////////////////////////////////

void ExecutableFileBuilder::patchHeader() {
    Byte* buffer_to_write_to = this->mem_reg_manager->loadCommandsMemoryRegion->region.data;

    uint32_t offset_to_write_to = 0; // The header is always at the beginning.
    uint32_t size_to_write = sizeof(MachHeader64);

    // Patching...
    // (Modify this->output_macho.header as needed)

    memcpy(buffer_to_write_to + offset_to_write_to, this->output_macho.header, size_to_write);
}


// Just an example
void ExecutableFileBuilder::patchPageZeroSegment() {
    if (auto page_zero_seg_index = this->getIndexOfSegmentWithName(SEG_PAGEZERO)) {

        SegmentHandle* page_zero_seg = (*this->output_macho.segment_handles)[*page_zero_seg_index];
        
        static_cast<SegmentCommand64*>(page_zero_seg->load_command)->nsects	= 2;      // Just to illustrate how to patch!

        this->patchLoadCommand("LC_SEGMENT_64:__PAGEZERO",
                                reinterpret_cast<Byte*>(page_zero_seg->load_command),
                                sizeof(SegmentCommand64)); 

    }
}

// We will want to update the two fields of each linkedit_data_load_command:
//linkedit_data_load_command->dataoff = ...				   // file offset of data in __LINKEDIT segment 
//linkedit_data_load_command->datasize = ...			   // file size of data in __LINKEDIT segment
void ExecutableFileBuilder::patchLinkeditDataCommand(uint32_t cmd, uint32_t dataoff, uint32_t datasize) {

    assert(isLinkeditDataCommand(cmd));

    std::string key_for_linkedit_data_cmd = macroToString[cmd];

    LinkeditDataCommandHandle* handle = this->output_macho.getLinkeditDataCmdHandleForCmd(cmd);

    if (handle) {
        handle->load_command->dataoff = dataoff;
        handle->load_command->datasize = datasize;

        this->patchLoadCommand(key_for_linkedit_data_cmd,
            reinterpret_cast<Byte*>(handle->load_command),
            sizeof(LinkeditDataCommand)); 
    }
}



void ExecutableFileBuilder::patchLoadCommand(std::string key_in_region, Byte* new_data_for_lc, uint64_t size_new_data) {

    Byte* buffer_to_write_to = this->mem_reg_manager->loadCommandsMemoryRegion->region.data;

    assert(buffer_to_write_to != nullptr);
    assert(this->mem_reg_manager->loadCommandsMemoryRegion != nullptr);
    assert(this->mem_reg_manager->loadCommandsMemoryRegion->offsets.count(key_in_region) > 0);
    
    uint32_t offset_to_write_to = this->mem_reg_manager->loadCommandsMemoryRegion->offsets[key_in_region].offset;
    uint32_t size_to_write = this->mem_reg_manager->loadCommandsMemoryRegion->offsets[key_in_region].size;
    assert(size_new_data == size_to_write);

    memcpy(buffer_to_write_to + offset_to_write_to, new_data_for_lc, size_to_write);
}

void ExecutableFileBuilder::patchEntryPointCommand(uint64_t new_entryoff) {

    std::string key_for_entry_point_cmd = macroToString[LC_MAIN];

    EntryPointCommandHandle* entry_point_handle = this->output_macho.entry_point_handle;
    if (entry_point_handle) {
        static_cast<EntryPointCommand*>(entry_point_handle->load_command)->entryoff = new_entryoff;
        this->patchLoadCommand(key_for_entry_point_cmd,
            reinterpret_cast<Byte*>(entry_point_handle->load_command),
            sizeof(EntryPointCommand)); 
    }
}


/////////////////////////////////// ExecutableFileBuilder : Misc ////////////////////////////////

std::optional<uint64_t> ExecutableFileBuilder::getIndexOfSegmentWithName(std::string segname) {
    for (uint64_t i = 0; i < this->output_macho.segment_handles->size(); i++) {
        SegmentHandle* seg_handle = (*this->output_macho.segment_handles)[i];
        if (seg_handle->segname == segname) {
            return i;
        }
    }
    return {};
}








/////////////////////////////////// ExecutableFileBuilder : Debug ////////////////////////////////

void ExecutableFileBuilder::debugOffsetsMap() {
    std::cout << "There are " << this->mem_reg_manager->loadCommandsMemoryRegion->offsets.size() << " entries" << std::endl;
    for (auto const& [cmd_str, of_sz] : this->mem_reg_manager->loadCommandsMemoryRegion->offsets) {
        std::cout << cmd_str        // string (key)
                << " : "  
                << of_sz.offset
                << ", " 
                << of_sz.size    
                << std::endl;
    }
}
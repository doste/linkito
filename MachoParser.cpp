#include "MachoParser.h"

MachoParser::MachoParser() {}

MachoParser::MachoParser(char* filename) {

    this->macho = new Macho(filename);

    this->buildLoadCommandsMemoryRegion();
    this->buildLoadCommands();
}   


void MachoParser::buildLoadCommands() {
    this->buildSegmentCommands();
    this->assignPayloadsToSections();

    this->buildLinkeditDataCommands();
    this->assignPayloadsToLinkeditBlobs();

    this->buildSymbolTable();
    this->buildStringTable();
    this->buildDySymbolTable();

    this->buildBuildVersionLoadCommand();
    this->buildDyLinkerCommand();
    this->buildLoadDylibCommandHandle();
    this->buildEntryPointCommand();
    this->buildUuidCommand();
    this->buildSourceVersionCommand();
}

void MachoParser::patchTextSeg() {
    Byte* buffer_to_read_from = this->macho->load_commands_mem_region.region;
    std::string key = "LC_SEGMENT_64:__TEXT";
    uint32_t offset_to_read_from = this->macho->load_commands_mem_region.offsets[key].offset;
    //std::cout << "offset_to_read_from" << offset_to_read_from << std::endl;
    uint32_t size_to_read = this->macho->load_commands_mem_region.offsets[key].size;
    //std::cout << "size_to_read" << size_to_read << std::endl;

    SegmentCommand64 patched_lc;
    patched_lc.vmaddr = 0xEEEEEEEEE;
    patched_lc.vmsize = 0xEEEEEEEEE;
    patched_lc.fileoff = 0xEEEEEEEEE;
    patched_lc.filesize = 0xEEEEEEEEE;
    patched_lc.maxprot = 0xEEEEEEEEE;
    patched_lc.initprot = 0xEEEEEEEEE;
    patched_lc.nsects = 0xEEEEEEE;
    patched_lc.flags = 0xEEEEEEEEE;
    patched_lc.cmd = 0xFF;

    memcpy(buffer_to_read_from + offset_to_read_from, &patched_lc, size_to_read);
}

/*
The idea with the Load Commands Region is to have a memory region where all the load commands reside.
This way when we want to obtain a given load command we read it from this region.
All we need to read a load command from here is its 'name'. Officially the load commands have no name per se, but
we use the 'cmd' field as a string. In the case where there are multiple load commands with the same 'cmd', for example
the load commands corresponding to segments (LC_SEGMENT_64) we append the segment name ('segname' field) to the string
representing its name.

The Load Commands Region is a dictionary, where
    - Key = Name of the load command.
    - Value = A pair of an offset and size to know where in the memory region the given load command resides.

Then, to obtain LoadCommand with name "LC_One", we just need to do:
        Byte* buffer_to_read_from = this->load_commands.region;
        uint32_t offset_to_read_from = this->load_commands.offsets["LC_One"].offset;
        uint32_t size_to_read = this->load_commands.offsets["LC_One"].size;
        memcpy(&(struct to put LC_One), buffer_to_read_from + offset_to_read_from, size_to_read); 
*/
void MachoParser::buildLoadCommandsMemoryRegion() {
    Byte* buf = this->macho->file.buffer;
    uint32_t offset_to_read_buffer = 0;
    // Skip the header
    offset_to_read_buffer += sizeof(MachHeader64);
    uint32_t cmd, cmdsize;

    this->macho->load_commands_mem_region = LoadCommandsRegion();

    this->macho->load_commands_mem_region.region = (Byte*)malloc(sizeof(Byte) * this->macho->header->sizeofcmds);
    uint32_t offset_mem_region = 0;

    while (offset_to_read_buffer < (sizeof(MachHeader64) + this->macho->header->sizeofcmds)) {

        // Save the offset of the beginning of this load command:
        uint32_t this_load_command_offset = offset_to_read_buffer;

        // First obtain the cmd and cmdsize:
        memcpy(&cmd, buf + offset_to_read_buffer, sizeof(uint32_t));
        offset_to_read_buffer += sizeof(uint32_t);
        memcpy(&cmdsize, buf + offset_to_read_buffer, sizeof(uint32_t));
        offset_to_read_buffer += sizeof(uint32_t);

        // Subtract so when we dump to our own memory region, we start at the beginning of the command, and not skip the cmd and cmdsize fields:
        offset_to_read_buffer -= (sizeof(uint32_t) + sizeof(uint32_t));
        memcpy(this->macho->load_commands_mem_region.region + offset_mem_region, buf + offset_to_read_buffer, cmdsize);
        offset_mem_region += cmdsize;

        // For example if cmd == LC_BUILD_VERSION then key will be "LC_BUILD_VERSION":
        std::string key_cmd_name = std::string{macroToString[cmd]};
        OffsetAndSize offset_and_size;
        if (cmd == LC_SEGMENT_64) { // If it's a Segment, we need to concatenate its segname to the key string
            char segname[16] = {0}; // The segname field follows directly the cmdsize field.
            uint32_t segname_offset = offset_to_read_buffer + (sizeof(uint32_t) + sizeof(uint32_t));
            memcpy(segname, buf + segname_offset, 16);
            key_cmd_name = key_cmd_name + ":";
            key_cmd_name = key_cmd_name + std::string{segname};
        }
        // this_load_command_offset is with respect to the beginning of the file. What we need to store is the offset
        // with respect to the memory region of the load commands we are building, so to correctly do that we
        // need to subtract the header that is at the beginning of the file.
        // The load commands follow it directly after.
        offset_and_size = OffsetAndSize(this_load_command_offset - sizeof(MachHeader64), cmdsize);

        this->macho->load_commands_mem_region.offsets.insert(std::make_pair(key_cmd_name, offset_and_size));

        // loader.h: To advance to the next load command the cmdsize can be added to the offset or pointer of the current load command.
        offset_to_read_buffer += cmdsize;
    }
}


void MachoParser::buildBuildVersionLoadCommand() {
    std::string key = macroToString[LC_BUILD_VERSION];
    Byte* buffer_to_read_from = this->macho->load_commands_mem_region.region;
    uint32_t offset_to_read_from = this->macho->load_commands_mem_region.offsets[key].offset;
    uint32_t size_to_read = this->macho->load_commands_mem_region.offsets[key].size;

    this->macho->build_version_handle = new BuildVersionHandle();
    this->macho->build_version_handle->load_command = new BuildVersionCommand();
    // First we only copy the struct itself (cmdsize includes the following struct(s)):
    memcpy(this->macho->build_version_handle->load_command, buffer_to_read_from + offset_to_read_from, sizeof(BuildVersionCommand));

    // And now the struct build_tool_version that follows it (the number of them is given by the ntools field).
    uint32_t build_tool_version_offset = offset_to_read_from + sizeof(BuildVersionCommand);
    this->macho->build_version_handle->tool_versions.reserve(static_cast<BuildVersionCommand*>(this->macho->build_version_handle->load_command)->ntools * sizeof(struct build_tool_version));
    for (size_t i = 0; i < static_cast<BuildVersionCommand*>(this->macho->build_version_handle->load_command)->ntools; i++) {
        memcpy(&(this->macho->build_version_handle->tool_versions[i]), buffer_to_read_from + build_tool_version_offset, sizeof(struct build_tool_version));
        build_tool_version_offset += sizeof(struct build_tool_version);
    }
}


void MachoParser::buildDyLinkerCommand() {
    std::string key = macroToString[LC_LOAD_DYLINKER];
    Byte* buffer_to_read_from = this->macho->load_commands_mem_region.region;
    uint32_t offset_to_read_from = this->macho->load_commands_mem_region.offsets[key].offset;
    uint32_t size_to_read = this->macho->load_commands_mem_region.offsets[key].size;

    this->macho->load_dylinker_handle = new LoadDyLinkerCommandHandle();
    this->macho->load_dylinker_handle->load_command = new LoadDyLinkerCommand();

    // First we copy the struct itself:
    memcpy(this->macho->load_dylinker_handle->load_command, buffer_to_read_from + offset_to_read_from, sizeof(LoadDyLinkerCommand));

    // And then for the pathname field , we need to access the field name.offset, this offset gives us the name (starting from the beginning of the struct dylinker_command)
    uint32_t pathname_string_len = this->macho->load_dylinker_handle->load_command->cmdsize - sizeof(LoadDyLinkerCommand); // Because cmdsize includes pathname string.
    this->macho->load_dylinker_handle->pathname = (char*)malloc(sizeof(char) * pathname_string_len);
    memcpy(this->macho->load_dylinker_handle->pathname, buffer_to_read_from + offset_to_read_from + sizeof(LoadDyLinkerCommand), pathname_string_len);
}

void MachoParser::buildEntryPointCommand() {
    std::string key = macroToString[LC_MAIN];
    Byte* buffer_to_read_from = this->macho->load_commands_mem_region.region;
    uint32_t offset_to_read_from = this->macho->load_commands_mem_region.offsets[key].offset;
    uint32_t size_to_read = this->macho->load_commands_mem_region.offsets[key].size;

    this->macho->entry_point_handle = new EntryPointCommandHandle();
    this->macho->entry_point_handle->load_command = new EntryPointCommand();

    // Just copy the struct itself:
    memcpy(this->macho->entry_point_handle->load_command, buffer_to_read_from + offset_to_read_from, sizeof(EntryPointCommand));
}

void MachoParser::buildUuidCommand() {
    std::string key = macroToString[LC_UUID];
    Byte* buffer_to_read_from = this->macho->load_commands_mem_region.region;
    uint32_t offset_to_read_from = this->macho->load_commands_mem_region.offsets[key].offset;
    uint32_t size_to_read = this->macho->load_commands_mem_region.offsets[key].size;

    this->macho->uuid_handle = new UuidCommandHandle();
    this->macho->uuid_handle->load_command = new UuidCommand();

    // Just copy the struct itself:
    memcpy(this->macho->uuid_handle->load_command, buffer_to_read_from + offset_to_read_from, sizeof(UuidCommand));
}

void MachoParser::buildSourceVersionCommand() {
    std::string key = macroToString[LC_SOURCE_VERSION];
    Byte* buffer_to_read_from = this->macho->load_commands_mem_region.region;
    uint32_t offset_to_read_from = this->macho->load_commands_mem_region.offsets[key].offset;
    uint32_t size_to_read = this->macho->load_commands_mem_region.offsets[key].size;

    this->macho->source_version_handle = new SourceVersionCommandHandle();
    this->macho->source_version_handle->load_command = new SourceVersionCommand();

    // Just copy the struct itself:
    memcpy(this->macho->source_version_handle->load_command, buffer_to_read_from + offset_to_read_from, sizeof(SourceVersionCommand));
}

void MachoParser::buildLoadDylibCommandHandle() {

    std::string key = macroToString[LC_LOAD_DYLIB];
    Byte* buffer_to_read_from = this->macho->load_commands_mem_region.region;
    uint32_t offset_to_read_from = this->macho->load_commands_mem_region.offsets[key].offset;
    uint32_t size_to_read = this->macho->load_commands_mem_region.offsets[key].size;

    this->macho->load_dylib_handle = new LoadDylibCommandHandle();
    this->macho->load_dylib_handle->load_command = new LoadDylibCommand();

    // First copy the struct dylib_command:
    memcpy(this->macho->load_dylib_handle->load_command, buffer_to_read_from + offset_to_read_from, sizeof(LoadDylibCommand));
    // And then the struct dylib, that is just after:
    uint32_t size_of_cmd_and_cmdsize_fields = sizeof(uint32_t) + sizeof(uint32_t);
    memcpy(&(static_cast<LoadDylibCommand*>(this->macho->load_dylib_handle->load_command)->dylib), buffer_to_read_from + offset_to_read_from + size_of_cmd_and_cmdsize_fields, sizeof(struct dylib));

    // Finally, the pathname, its offset with respect to the beginning of the struct DylibCommand is given by name.offset:
    uint32_t library_pathname_string_len = this->macho->load_dylib_handle->load_command->cmdsize - sizeof(LoadDylibCommand); // Because cmdsize includes pathname string.
    this->macho->load_dylib_handle->library_path_name = (char*)malloc(sizeof(char) * library_pathname_string_len);
    memcpy(this->macho->load_dylib_handle->library_path_name,
            buffer_to_read_from + offset_to_read_from + sizeof(LoadDylibCommand),
            library_pathname_string_len);

}

// Traverse the load_commands.offsets map and look for those keys that start with 'LC_SEGMENT_64'.
std::vector<std::string> MachoParser::getSegmentLoadCommandsPresentInTheMap() {
    std::vector<std::string> segment_load_commands;
    for(std::map<std::string, OffsetAndSize>::iterator iter = this->macho->load_commands_mem_region.offsets.begin(); iter != this->macho->load_commands_mem_region.offsets.end(); ++iter) {
        std::string cmd = iter->first;
        if (cmd.rfind(macroToString[LC_SEGMENT_64], 0) == 0) {
            segment_load_commands.push_back(cmd);
        }
    }
    return segment_load_commands;
}


void MachoParser::buildSegmentCommands() {
    std::vector<std::string> segment_load_commands = getSegmentLoadCommandsPresentInTheMap();

    //std::cout << "segment_load_commands ESSSS" << std::endl;
    //for(std::string seg : segment_load_commands) {
    //    std::cout << seg << std::endl;
    //}
    //std::cout << "--------------------" << std::endl;

    for (std::string key : segment_load_commands) {
        Byte* buffer_to_read_from = this->macho->load_commands_mem_region.region;
        uint32_t offset_to_read_from = this->macho->load_commands_mem_region.offsets[key].offset;
        uint32_t size_to_read = this->macho->load_commands_mem_region.offsets[key].size;

        SegmentHandle* seg_handle = new SegmentHandle();
        seg_handle->load_command = new SegmentCommand64();
        memcpy(seg_handle->load_command, buffer_to_read_from + offset_to_read_from, sizeof(SegmentCommand64)); // cmdsize includes sizeof section_64 structs

        seg_handle->segname = static_cast<SegmentCommand64*>(seg_handle->load_command)->segname;

        if (static_cast<SegmentCommand64*>(seg_handle->load_command)->nsects != 0) {  // If it has sections following it, we add them to the vector
            seg_handle->sections.reserve(static_cast<SegmentCommand64*>(seg_handle->load_command)->nsects);
            
            // The sections follow the struct segment_command_64
            uint32_t section_offset = offset_to_read_from + sizeof(SegmentCommand64);

            // Build and add each section to the vector
            for (size_t i = 0; i < static_cast<SegmentCommand64*>(seg_handle->load_command)->nsects; i++) {
                SectionHandle* sect = new SectionHandle();
                memcpy(sect->section, buffer_to_read_from + section_offset, sizeof(Section64));
                seg_handle->sections.push_back(sect);
                section_offset += sizeof(Section64);
            }
        }
        this->macho->segment_handles->push_back(seg_handle);
    }
} 



void MachoParser::buildLinkeditDataCommands() {
    const std::vector<uint32_t> linkedit_data_commands = getLinkeditCommands();
    for (uint32_t linkedit_data_cmd : linkedit_data_commands) {
        std::string key = macroToString[linkedit_data_cmd];

        LinkeditDataCommandHandle* linkedit_data = new LinkeditDataCommandHandle();

        Byte* buffer_to_read_from = this->macho->load_commands_mem_region.region;
        uint32_t offset_to_read_from = this->macho->load_commands_mem_region.offsets[key].offset;
        uint32_t size_to_read = this->macho->load_commands_mem_region.offsets[key].size;
        memcpy(linkedit_data->load_command, buffer_to_read_from + offset_to_read_from, size_to_read);

        this->macho->linkedit_data_handles->push_back(linkedit_data);
    }
}


void MachoParser::assignPayloadsToLinkeditBlobs() {

    for (LinkeditDataCommandHandle* linkedit_data : *this->macho->linkedit_data_handles) {
        if (linkedit_data->load_command->datasize > 0) {
            linkedit_data->payload = (Byte*)malloc(sizeof(Byte) * linkedit_data->load_command->datasize);
            memcpy(linkedit_data->payload, this->macho->file.buffer + linkedit_data->load_command->dataoff, linkedit_data->load_command->datasize);
        }
    }
}


void MachoParser::assignPayloadsToSections() {

    for (SegmentHandle* seg_handle : *this->macho->segment_handles) {
        for (SectionHandle* sect : seg_handle->sections) {
            if (sect->section) {
                sect->payload = (Byte*)malloc(sizeof(Byte) * sect->section->size);
                memcpy(sect->payload, this->macho->file.buffer + sect->section->offset, sect->section->size);
            }
        }
    }
}




void MachoParser::buildStringTable() {
    Byte* buf = this->macho->file.buffer;
    uint32_t strtab_offset = static_cast<SymTabCommand*>(this->macho->symtab.symtab_command_handle->load_command)->stroff;
    uint32_t strtab_size = static_cast<SymTabCommand*>(this->macho->symtab.symtab_command_handle->load_command)->strsize;
    char* input_string_table = (char*)malloc(sizeof(char) * strtab_size);
    memcpy(input_string_table, buf + strtab_offset, strtab_size);

    this->macho->symtab.strtab = StringTable();

    std::vector<size_t> string_start_indices;

    // Extract the strings from the input string table:
    size_t i = 0;
    char* ptr = input_string_table;
    size_t idx_start_of_string = 0;
    while (idx_start_of_string < strtab_size) {

        bool first_char_in_string = true;   // To save the index from where the string starts

        while(ptr && *ptr != '\0') {
            if (first_char_in_string) {
                string_start_indices.push_back(idx_start_of_string);
                first_char_in_string = false;
            }
            idx_start_of_string++;
            ptr++;
        }
        
        idx_start_of_string++;
        ptr++;
    }

    this->macho->symtab.strtab.entries.reserve(strtab_size / sizeof(StringTableEntry));

    for (size_t idx_start_of_string : string_start_indices) {
        if (idx_start_of_string != 0) {
            //std::cout << "idx_start_of_string: " << idx_start_of_string << std::endl;
            StringTableEntry entry_i = {
                .string = input_string_table + idx_start_of_string,
                .index_into_table = idx_start_of_string};
            
            this->macho->symtab.strtab.entries.push_back(entry_i);
        }
    }

}


void MachoParser::buildSymbolTable() {
    std::string key = macroToString[LC_SYMTAB];
    Byte* buffer_to_read_from = this->macho->load_commands_mem_region.region;
    uint32_t offset_to_read_from = this->macho->load_commands_mem_region.offsets[key].offset;
    uint32_t size_to_read = this->macho->load_commands_mem_region.offsets[key].size;

    this->macho->symtab = SymbolTable();

    this->macho->symtab.symtab_command_handle = new SymTabCommandHandle();
    this->macho->symtab.symtab_command_handle->load_command = new SymTabCommand();
    // First we only copy the SymTabCommand struct itself:
    memcpy(this->macho->symtab.symtab_command_handle->load_command,
            buffer_to_read_from + offset_to_read_from,
            sizeof(SymTabCommand));

    // And now the symbol table entries, each of them is a struct nlist_64.
    // The symbol table will have symtab_cmd->nsyms of these structs.
    // To obtain them we need to move to symtab_cmd->symoff, but
    // this offset is with respect to the whole file not the load commands region, so
    // we need to use this->file.buffer as 'base'.
    Byte* buf = this->macho->file.buffer;
    uint32_t nsyms = static_cast<SymTabCommand*>(this->macho->symtab.symtab_command_handle->load_command)->nsyms;
    uint32_t symtab_offset = static_cast<SymTabCommand*>(this->macho->symtab.symtab_command_handle->load_command)->symoff;

    struct nlist_64* input_symbol_table = (struct nlist_64*)malloc(sizeof(struct nlist_64) * nsyms);
    memcpy(input_symbol_table, buf + symtab_offset, sizeof(struct nlist_64) * nsyms);

    this->macho->symtab.entries.reserve(nsyms);

    for (size_t i = 0; i < nsyms; i++) {
        struct nlist_64 ith_input_entry = input_symbol_table[i];
        SymbolTableEntry entry_i = {
                                            .index_into_string_table = ith_input_entry.n_un.n_strx,
                                            .type = get_own_symbol_table_entry_type(ith_input_entry.n_type),
                                            .section_number = ith_input_entry.n_sect,
                                            .desc = ith_input_entry.n_desc,
                                            .value = ith_input_entry.n_value,
        };
        this->macho->symtab.entries.push_back(entry_i);
    }
}


void MachoParser::buildDySymbolTable() {
    std::string key = macroToString[LC_DYSYMTAB];
    Byte* buffer_to_read_from = this->macho->load_commands_mem_region.region;
    uint32_t offset_to_read_from = this->macho->load_commands_mem_region.offsets[key].offset;
    uint32_t size_to_read = this->macho->load_commands_mem_region.offsets[key].size;

    this->macho->dysymtab_handle = new DySymTabHandle();
    this->macho->dysymtab_handle->load_command = new DySymTabCommand();

    // Just copy the struct itself:
    memcpy(this->macho->dysymtab_handle->load_command, buffer_to_read_from + offset_to_read_from, sizeof(DySymTabCommand));
}
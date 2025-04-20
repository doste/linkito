#include "Macho.h"
#include <set>
#include <tuple>



Macho::Macho(char* filename, const char* pathname) {
    FILE* fptr = open_macho_file(pathname);
    read_macho_header(fptr, &this->header);
    switch (this->header.filetype) {
        case MH_OBJECT:
            this->filetype = RelocatableObjectFile;
            break;
        case MH_EXECUTE:
            this->filetype = ExecutableFile;
            break;
        case MH_DYLIB:
            this->filetype = DynamicLibrary;
            break;
        default:
            fprintf(stderr, "Error: Not supported filetype\n");
            exit(1);
    }

    this->file = File(filename, File::get_file_size(fptr), fptr);
    this->file.fill_buffer();
    this->segment_handles = std::vector<SegmentHandle*>();
    this->linkedit_data_handles = std::vector<LinkeditDataCommandHandle*>();

    this->buildLoadCommandsMemoryRegion();
    
}


/*
The idea with the Load Commands Region is to have a memory region where all the load commands reside.
This way when we want to obtain a given load command we read it from this region.
All we need to read a load command from here is its 'name'. Oficially the load commands have no name per se, but
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
void Macho::buildLoadCommandsMemoryRegion() {
    Byte* buf = this->file.buffer;
    uint32_t offset_to_read_buffer = 0;
    // Skip the header
    offset_to_read_buffer += sizeof(struct mach_header_64);
    uint32_t cmd, cmdsize;

    this->load_commands_mem_region = LoadCommandsRegion();

    this->load_commands_mem_region.region = (Byte*)malloc(sizeof(Byte) * this->header.sizeofcmds);
    uint32_t offset_mem_region = 0;

    while (offset_to_read_buffer < (sizeof(struct mach_header_64) + this->header.sizeofcmds)) {

        // Save the offset of the beginning of this load command:
        uint32_t this_load_command_offset = offset_to_read_buffer;

        // First obtain the cmd and cmdsize:
        memcpy(&cmd, buf + offset_to_read_buffer, sizeof(uint32_t));
        offset_to_read_buffer += sizeof(uint32_t);
        memcpy(&cmdsize, buf + offset_to_read_buffer, sizeof(uint32_t));
        offset_to_read_buffer += sizeof(uint32_t);

        // Subtract so when we dump to our own memory region, we start at the beginning of the command, and not skip the cmd and cmdsize fields:
        offset_to_read_buffer -= (sizeof(uint32_t) + sizeof(uint32_t));
        memcpy(this->load_commands_mem_region.region + offset_mem_region, buf + offset_to_read_buffer, cmdsize);
        offset_mem_region += cmdsize;

        // For example if cmd == LC_BUILD_VERSION then key will be "LC_BUILD_VERSION":
        std::string key_cmd_name = std::string{macroToString[cmd]};
        OffsetAndSize offset_and_size;
        if (cmd == LC_SEGMENT_64) { // If it's a Segment, we need to concatenate its segname to the key string
            char segname[16] = {0}; // The segname field follows directly the cmdsize field.
            uint32_t segname_offset = offset_to_read_buffer + (sizeof(uint32_t) + sizeof(uint32_t));
            memcpy(segname, buf + segname_offset, 16);
            //std::cout << "segname >>>>>>>>" << segname << std::endl;
            key_cmd_name = key_cmd_name + ":";
            key_cmd_name = key_cmd_name + std::string{segname};
        }
        // this_load_command_offset is with respect to the beginning of the file. What we need to store is the offset
        // with respect to the memory region of the load commands we are building, so to correctly do that we
        // need to subtract the header that is at the beginning of the file.
        // The load commands follow it directly after.
        offset_and_size = OffsetAndSize(this_load_command_offset - sizeof(struct mach_header_64), cmdsize);

        this->load_commands_mem_region.offsets.insert(std::make_pair(key_cmd_name, offset_and_size));

        // loader.h: To advance to the next load command the cmdsize can be added to the offset or pointer of the current load command.
        offset_to_read_buffer += cmdsize;
    }
}

void Macho::buildLoadCommands() {
    this->buildSegmentCommands();
    this->assignPayloadsToSections();

    this->buildLinkeditDataCommands();
    this->assignPayloadsToLinkeditBlobs();

    this->buildSymbolTable();
    this->buildStringTable();

    this->buildBuildVersionLoadCommand();
    this->buildDyLinkerCommand();
    this->buildLoadDylibCommandHandle();
    this->buildEntryPointCommand();
    this->buildUuidCommand();
    this->buildSourceVersionCommand();
}

void Macho::buildBuildVersionLoadCommand() {
    std::string key = macroToString[LC_BUILD_VERSION];
    Byte* buffer_to_read_from = this->load_commands_mem_region.region;
    uint32_t offset_to_read_from = this->load_commands_mem_region.offsets[key].offset;
    uint32_t size_to_read = this->load_commands_mem_region.offsets[key].size;

    this->build_version_handle = new BuildVersionHandle();
    this->build_version_handle->load_command = new BuildVersionCommand();
    // First we only copy the struct itself (cmdsize includes the following struct(s)):
    memcpy(this->build_version_handle->load_command, buffer_to_read_from + offset_to_read_from, sizeof(BuildVersionCommand));

    // And now the struct build_tool_version that follows it (the number of them is given by the ntools field).
    uint32_t build_tool_version_offset = offset_to_read_from + sizeof(BuildVersionCommand);
    this->build_version_handle->tool_versions.reserve(this->build_version_handle->load_command->ntools * sizeof(struct build_tool_version));
    for (size_t i = 0; i < this->build_version_handle->load_command->ntools; i++) {
        memcpy(&(this->build_version_handle->tool_versions[i]), buffer_to_read_from + build_tool_version_offset, sizeof(struct build_tool_version));
        build_tool_version_offset += sizeof(struct build_tool_version);
    }
}

void Macho::buildDyLinkerCommand() {
    std::string key = macroToString[LC_LOAD_DYLINKER];
    Byte* buffer_to_read_from = this->load_commands_mem_region.region;
    uint32_t offset_to_read_from = this->load_commands_mem_region.offsets[key].offset;
    uint32_t size_to_read = this->load_commands_mem_region.offsets[key].size;

    this->load_dylinker_handle = new LoadDyLinkerCommandHandle();
    this->load_dylinker_handle->load_command = new DyLinkerCommand();

    // First we copy the struct itself:
    memcpy(this->load_dylinker_handle->load_command, buffer_to_read_from + offset_to_read_from, sizeof(DyLinkerCommand));

    // And then for the pathname field , we need to access the field name.offset, this offset gives us the name (starting from the beginning of the struct dylinker_command)
    uint32_t pathname_string_len = this->load_dylinker_handle->load_command->cmdsize - sizeof(DyLinkerCommand); // Because cmdsize includes pathname string.
    this->load_dylinker_handle->pathname = (char*)malloc(sizeof(char) * pathname_string_len);
    memcpy(this->load_dylinker_handle->pathname, buffer_to_read_from + offset_to_read_from + sizeof(DyLinkerCommand), pathname_string_len);
}

void Macho::buildEntryPointCommand() {
    std::string key = macroToString[LC_MAIN];
    Byte* buffer_to_read_from = this->load_commands_mem_region.region;
    uint32_t offset_to_read_from = this->load_commands_mem_region.offsets[key].offset;
    uint32_t size_to_read = this->load_commands_mem_region.offsets[key].size;

    this->entry_point_handle = new EntryPointCommandHandle();
    this->entry_point_handle->load_command = new EntryPointCommand();

    // Just copy the struct itself:
    memcpy(this->entry_point_handle->load_command, buffer_to_read_from + offset_to_read_from, sizeof(EntryPointCommand));
}

void Macho::buildUuidCommand() {
    std::string key = macroToString[LC_UUID];
    Byte* buffer_to_read_from = this->load_commands_mem_region.region;
    uint32_t offset_to_read_from = this->load_commands_mem_region.offsets[key].offset;
    uint32_t size_to_read = this->load_commands_mem_region.offsets[key].size;

    this->uuid_handle = new UuidCommandCommandHandle();
    this->uuid_handle->load_command = new UuidCommand();

    // Just copy the struct itself:
    memcpy(this->uuid_handle->load_command, buffer_to_read_from + offset_to_read_from, sizeof(UuidCommand));
}

void Macho::buildSourceVersionCommand() {
    std::string key = macroToString[LC_SOURCE_VERSION];
    Byte* buffer_to_read_from = this->load_commands_mem_region.region;
    uint32_t offset_to_read_from = this->load_commands_mem_region.offsets[key].offset;
    uint32_t size_to_read = this->load_commands_mem_region.offsets[key].size;

    this->source_version_handle = new SourceVersionCommandHandle();
    this->source_version_handle->load_command = new SourceVersionCommand();

    // Just copy the struct itself:
    memcpy(this->source_version_handle->load_command, buffer_to_read_from + offset_to_read_from, sizeof(SourceVersionCommand));
}

void Macho::buildLoadDylibCommandHandle() {

    std::string key = macroToString[LC_LOAD_DYLIB];
    Byte* buffer_to_read_from = this->load_commands_mem_region.region;
    uint32_t offset_to_read_from = this->load_commands_mem_region.offsets[key].offset;
    uint32_t size_to_read = this->load_commands_mem_region.offsets[key].size;

    this->load_dylib_handle = new LoadDylibCommandHandle();
    this->load_dylib_handle->load_command = new DylibCommand();

    // First copy the struct dylib_command:
    memcpy(this->load_dylib_handle->load_command, buffer_to_read_from + offset_to_read_from, sizeof(DylibCommand));
    // And then the struct dylib, that is just after:
    uint32_t size_of_cmd_and_cmdsize_fields = sizeof(uint32_t) + sizeof(uint32_t);
    memcpy(&(this->load_dylib_handle->load_command->dylib), buffer_to_read_from + offset_to_read_from + size_of_cmd_and_cmdsize_fields, sizeof(struct dylib));

    // Finally, the pathname, its offset with respect to the beginning of the struct DylibCommand is given by name.offset:
    uint32_t library_pathname_string_len = this->load_dylib_handle->load_command->cmdsize - sizeof(DylibCommand); // Because cmdsize includes pathname string.
    this->load_dylib_handle->library_path_name = (char*)malloc(sizeof(char) * library_pathname_string_len);
    memcpy(this->load_dylib_handle->library_path_name,
            buffer_to_read_from + offset_to_read_from + sizeof(DylibCommand),
            library_pathname_string_len);

}

// Traverse the load_commands.offsets map and look for those keys that start with 'LC_SEGMENT_64'.
std::vector<std::string> Macho::getSegmentLoadCommandsPresentInTheMap() {
    std::vector<std::string> segment_load_commands;
    for(std::map<std::string, OffsetAndSize>::iterator iter = this->load_commands_mem_region.offsets.begin(); iter != this->load_commands_mem_region.offsets.end(); ++iter) {
        std::string cmd = iter->first;
        if (cmd.rfind(macroToString[LC_SEGMENT_64], 0) == 0) {
            segment_load_commands.push_back(cmd);
        }
    }
    return segment_load_commands;
}

void Macho::buildSegmentCommands() {
    std::vector<std::string> segment_load_commands = getSegmentLoadCommandsPresentInTheMap();
    for (std::string key : segment_load_commands) {
        Byte* buffer_to_read_from = this->load_commands_mem_region.region;
        uint32_t offset_to_read_from = this->load_commands_mem_region.offsets[key].offset;
        uint32_t size_to_read = this->load_commands_mem_region.offsets[key].size;

        SegmentHandle* seg_handle = new SegmentHandle();
        seg_handle->load_command = new SegmentCommand64();
        memcpy(seg_handle->load_command, buffer_to_read_from + offset_to_read_from, sizeof(SegmentCommand64)); // cmdsize includes sizeof section_64 structs

        seg_handle->segname = (char*)malloc(sizeof(char) * 16);  // loader.h: The field segname of struct segment_command_64 is defined as 'char segname[16];'
        strcpy(seg_handle->segname, seg_handle->load_command->segname);

        if (seg_handle->load_command->nsects != 0) {  // If it has sections following it, we add them to the vector
            seg_handle->sections.reserve(seg_handle->load_command->nsects);
            
            // The sections follow the struct segment_command_64
            uint32_t section_offset = offset_to_read_from + sizeof(SegmentCommand64);

            // Build and add each section to the vector
            for (size_t i = 0; i < seg_handle->load_command->nsects; i++) {
                SectionWithPayload sect;
                sect.payload = nullptr;
                sect.section = new Section64();
                memcpy(sect.section, buffer_to_read_from + section_offset, sizeof(Section64));
                seg_handle->sections.push_back(sect);
                section_offset += sizeof(Section64);
            }
        }
        this->segment_handles.push_back(seg_handle);
    }
} 



const std::vector<uint32_t> getLinkeditCommands() {
    return {LC_CODE_SIGNATURE, LC_SEGMENT_SPLIT_INFO,
            LC_FUNCTION_STARTS, LC_DATA_IN_CODE,
            LC_DYLIB_CODE_SIGN_DRS,
            LC_LINKER_OPTIMIZATION_HINT,
            LC_DYLD_EXPORTS_TRIE,
            LC_DYLD_CHAINED_FIXUPS};
}

bool isLinkeditDataCommand(uint32_t input_cmd) {
    const std::vector<uint32_t> vector_linkedit_data_cmds = getLinkeditCommands();
    std::set<uint32_t> linkedit_data_cmds(vector_linkedit_data_cmds.begin(), vector_linkedit_data_cmds.end());
    return linkedit_data_cmds.count(input_cmd) > 0;
}

void Macho::buildLinkeditDataCommands() {
    const std::vector<uint32_t> linkedit_data_commands = getLinkeditCommands();
    for (uint32_t linkedit_data_cmd : linkedit_data_commands) {
        std::string key = macroToString[linkedit_data_cmd];

        LinkeditDataCommandHandle* linkedit_data = new LinkeditDataCommandHandle();
        linkedit_data->load_command = new LinkeditDataCommand();

        Byte* buffer_to_read_from = this->load_commands_mem_region.region;
        uint32_t offset_to_read_from = this->load_commands_mem_region.offsets[key].offset;
        uint32_t size_to_read = this->load_commands_mem_region.offsets[key].size;
        memcpy(linkedit_data->load_command, buffer_to_read_from + offset_to_read_from, size_to_read);

        this->linkedit_data_handles.push_back(linkedit_data);
    }
}


void Macho::assignPayloadsToLinkeditBlobs() {

    for (LinkeditDataCommandHandle* linkedit_data : this->linkedit_data_handles) {
        if (linkedit_data->load_command->datasize > 0) {
            linkedit_data->payload = (Byte*)malloc(sizeof(Byte) * linkedit_data->load_command->datasize);
            memcpy(linkedit_data->payload, this->file.buffer + linkedit_data->load_command->dataoff, linkedit_data->load_command->datasize);
        }
    }
}


void Macho::assignPayloadsToSections() {

    for (SegmentHandle* seg_handle : this->segment_handles) {
        for (SectionWithPayload& sect : seg_handle->sections) {
            if (sect.section) {
                sect.payload = (Byte*)malloc(sizeof(Byte) * sect.section->size);
                memcpy(sect.payload, this->file.buffer + sect.section->offset, sect.section->size);
            }
        }
    }
}




void Macho::buildStringTable() {
    Byte* buf = this->file.buffer;
    uint32_t strtab_offset = this->symtab.symtab_command_handle->load_command->stroff;
    uint32_t strtab_size = this->symtab.symtab_command_handle->load_command->strsize;
    char* input_string_table = (char*)malloc(sizeof(char) * strtab_size);
    memcpy(input_string_table, buf + strtab_offset, strtab_size);

    this->symtab.strtab = StringTable();

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

    this->symtab.strtab.entries.reserve(strtab_size / sizeof(StringTableEntry));

    for (size_t idx_start_of_string : string_start_indices) {
        if (idx_start_of_string != 0) {
            std::cout << "idx_start_of_string: " << idx_start_of_string << std::endl;
            StringTableEntry entry_i = {
                .string = input_string_table + idx_start_of_string,
                .index_into_table = idx_start_of_string};
            
            this->symtab.strtab.entries.push_back(entry_i);
        }
    }

}


void Macho::buildSymbolTable() {
    std::string key = macroToString[LC_SYMTAB];
    Byte* buffer_to_read_from = this->load_commands_mem_region.region;
    uint32_t offset_to_read_from = this->load_commands_mem_region.offsets[key].offset;
    uint32_t size_to_read = this->load_commands_mem_region.offsets[key].size;

    this->symtab = SymbolTable();

    this->symtab.symtab_command_handle = new SymTabCommandHandle();
    this->symtab.symtab_command_handle->load_command = new SymTabCommand();
    // First we only copy the SymTabCommand struct itself:
    memcpy(this->symtab.symtab_command_handle->load_command,
            buffer_to_read_from + offset_to_read_from,
            sizeof(SymTabCommand));

    // And now the symbol table entries, each of them is a struct nlist_64.
    // The symbol table will have symtab_cmd->nsyms of these structs.
    // To obtain them we need to move to symtab_cmd->symoff, but
    // this offset is with respect to the whole file not the load commands region, so
    // we need to use this->file.buffer as 'base'.
    Byte* buf = this->file.buffer;
    uint32_t nsyms = this->symtab.symtab_command_handle->load_command->nsyms;
    uint32_t symtab_offset = this->symtab.symtab_command_handle->load_command->symoff;

    struct nlist_64* input_symbol_table = (struct nlist_64*)malloc(sizeof(struct nlist_64) * nsyms);
    memcpy(input_symbol_table, buf + symtab_offset, sizeof(struct nlist_64) * nsyms);

    this->symtab.entries.reserve(nsyms);

    for (size_t i = 0; i < nsyms; i++) {
        struct nlist_64 ith_input_entry = input_symbol_table[i];
        SymbolTableEntry entry_i = {
                                            .index_into_string_table = ith_input_entry.n_un.n_strx,
                                            .type = get_own_symbol_table_entry_type(ith_input_entry.n_type),
                                            .section_number = ith_input_entry.n_sect,
                                            .desc = ith_input_entry.n_desc,
                                            .value = ith_input_entry.n_value,
        };
        this->symtab.entries.push_back(entry_i);
    }
}




void Macho::buildSegmentCommandsDEP() {
    Byte* buf = this->file.buffer;
    uint32_t offset_to_read_buffer = 0;
    // Skip the header
    offset_to_read_buffer += sizeof(struct mach_header_64);

    // Read the Load Command 0 (which according to loader.h is a segment with all the sections)
    // [citation needed that is always the first load command]
    // First, the two fields of the command:
    uint32_t cmd, cmdsize;

    while (offset_to_read_buffer < (sizeof(struct mach_header_64) + this->header.sizeofcmds)) {
        memcpy(&cmd, buf + offset_to_read_buffer, sizeof(uint32_t));
        offset_to_read_buffer += sizeof(uint32_t);

        memcpy(&cmdsize, buf + offset_to_read_buffer, sizeof(uint32_t));
        offset_to_read_buffer += sizeof(uint32_t);

        if (cmd == LC_SEGMENT_64) {
            SegmentHandle handle = SegmentHandle();
            offset_to_read_buffer -= (sizeof(uint32_t) + sizeof(uint32_t));
            SegmentCommand64* segcmd = (SegmentCommand64*)malloc(sizeof(SegmentCommand64));
            memcpy(segcmd, buf + offset_to_read_buffer, cmdsize); // cmdsize includes sizeof section_64 structs

            if (segcmd->nsects != 0) {  // If it has sections following it, we add them to the vector
                handle.sections.reserve(segcmd->nsects);
                // offset_to_read_buffer was left with cmdsize added, but in this case cmdsize includes the sizeof all following section_64
                
                uint32_t section_offset = offset_to_read_buffer + sizeof(SegmentCommand64);

                for (size_t i = 0; i < segcmd->nsects; i++) {
                    SectionWithPayload sect;
                    sect.payload = nullptr;
                    sect.section = (Section64*)malloc(sizeof(Section64));
                    memcpy(sect.section, buf + section_offset, sizeof(Section64));
                    handle.sections.push_back(sect);
                    section_offset += sizeof(Section64);
                }
            }
            handle.segcmd = segcmd;
            this->segment_commands.push_back(handle);

            offset_to_read_buffer += cmdsize;
            continue;
        }
        // Subtract so the next read can start at the beginning of the command, and not skip the cmd and cmdsize fields
        offset_to_read_buffer -= (sizeof(uint32_t) + sizeof(uint32_t));
        // loader.h: To advance to the next load command the cmdsize can be added to the offset or pointer of the current load command.
        offset_to_read_buffer += cmdsize;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// DEPRECATED
void Macho::buildLinkeditDataCommandsDEP() {
    Byte* buf = this->file.buffer;
    uint32_t offset_to_read_buffer = 0;
    // Skip the header
    offset_to_read_buffer += sizeof(struct mach_header_64);

    // Read the Load Command 0 (which according to loader.h is a segment with all the sections)
    // [citation needed that is always the first load command]
    // First, the two fields of the command:
    uint32_t cmd, cmdsize;

    while (offset_to_read_buffer < (sizeof(struct mach_header_64) + this->header.sizeofcmds)) {
        memcpy(&cmd, buf + offset_to_read_buffer, sizeof(uint32_t));
        offset_to_read_buffer += sizeof(uint32_t);

        memcpy(&cmdsize, buf + offset_to_read_buffer, sizeof(uint32_t));
        offset_to_read_buffer += sizeof(uint32_t);

        if (isLinkeditDataCommand(cmd)) {
            
            offset_to_read_buffer -= (sizeof(uint32_t) + sizeof(uint32_t));
            LinkeditCommandWithPayload linkedit_data = LinkeditCommandWithPayload();
            //linkedit_data.payload = nullptr;

            linkedit_data.command = (LinkeditDataCommand*)malloc(sizeof(LinkeditDataCommand));
            memcpy(linkedit_data.command, buf + offset_to_read_buffer, cmdsize);

            this->linkedit_data.push_back(linkedit_data);

            offset_to_read_buffer += cmdsize;
            continue;
        }

        // Subtract so the next read can start at the beginning of the command, and not skip the cmd and cmdsize fields
        offset_to_read_buffer -= (sizeof(uint32_t) + sizeof(uint32_t));
        // loader.h: To advance to the next load command the cmdsize can be added to the offset or pointer of the current load command.
        offset_to_read_buffer += cmdsize;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void copyFromBufferWhileAdvancingOffset(void* dest, Byte* src, uint32_t* offset, uint32_t size) {
    memcpy(dest, src + *offset, size);
    *offset += size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// DEPRECATED
void Macho::buildBuildVersionLoadCommandDEP() {
    Byte* buf = this->file.buffer;
    uint32_t offset_to_read_buffer = 0;
    // Skip the header
    offset_to_read_buffer += sizeof(struct mach_header_64);
    uint32_t cmd, cmdsize;

    while (offset_to_read_buffer < (sizeof(struct mach_header_64) + this->header.sizeofcmds)) {
        memcpy(&cmd, buf + offset_to_read_buffer, sizeof(uint32_t));
        offset_to_read_buffer += sizeof(uint32_t);

        memcpy(&cmdsize, buf + offset_to_read_buffer, sizeof(uint32_t));
        offset_to_read_buffer += sizeof(uint32_t);

        if (cmd == LC_BUILD_VERSION) {
            
            offset_to_read_buffer -= (sizeof(uint32_t) + sizeof(uint32_t));
            this->build_version = BuildVersion();

            this->build_version.command = (BuildVersionCommand*)malloc(sizeof(BuildVersionCommand));
            memcpy(this->build_version.command, buf + offset_to_read_buffer, cmdsize);

            uint32_t build_tool_version_offset = offset_to_read_buffer + sizeof(BuildVersionCommand);

            this->build_version.tool_versions.reserve(this->build_version.command->ntools * sizeof(struct build_tool_version));

            for (size_t i = 0; i < this->build_version.command->ntools; i++) {
                memcpy(&(this->build_version.tool_versions[i]), buf + build_tool_version_offset, sizeof(struct build_tool_version));
                build_tool_version_offset += sizeof(struct build_tool_version);
            }

            offset_to_read_buffer += cmdsize;
            continue;
        }

        // Subtract so the next read can start at the beginning of the command, and not skip the cmd and cmdsize fields
        offset_to_read_buffer -= (sizeof(uint32_t) + sizeof(uint32_t));
        // loader.h: To advance to the next load command the cmdsize can be added to the offset or pointer of the current load command.
        offset_to_read_buffer += cmdsize;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void buildLoadCommand(uint32_t cmd, uint32_t cmdsize, Byte* buffer, uint32_t* offset) {
    SegmentHandle handle = SegmentHandle();
    *offset -= (sizeof(uint32_t) + sizeof(uint32_t));
    SegmentCommand64* segcmd = (SegmentCommand64*)malloc(sizeof(SegmentCommand64));
    memcpy(segcmd, buffer + *offset, cmdsize); // cmdsize includes sizeof section_64 structs

    if (segcmd->nsects != 0) {  // If it has sections following it, we add them to the vector
        handle.sections.reserve(segcmd->nsects);
        // offset_to_read_buffer was left with cmdsize added, but in this case cmdsize includes the sizeof all following section_64
        
        uint32_t section_offset = *offset + sizeof(SegmentCommand64);

        for (size_t i = 0; i < segcmd->nsects; i++) {
            SectionWithPayload sect;
            sect.payload = nullptr;
            sect.section = (Section64*)malloc(sizeof(Section64));
            memcpy(sect.section, buffer + section_offset, sizeof(Section64));
            handle.sections.push_back(sect);
            section_offset += sizeof(Section64);
        }
    }
    handle.segcmd = segcmd;
    //this->segment_commands.push_back(handle);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


LoadCommandsRegion memreg = macho.load_commands;

struct build_version_command lc;
struct build_tool_version btv;
std::string key = macroToString[LC_BUILD_VERSION];
uint32_t size = memreg.offsets[key].size;
uint32_t offset = memreg.offsets[key].offset;
memcpy(&lc, memreg.region + offset, sizeof(struct build_version_command));
std::cout << "LC_BUILD_VERSION ntools: " << lc.ntools << std::endl;
std::cout << "LC_BUILD_VERSION platform: " << lc.platform << std::endl;
std::cout << "LC_BUILD_VERSION cmdsize: " << lc.cmdsize << std::endl;
uint32_t new_offset = offset + sizeof(struct build_version_command);
std::cout << "new_offset: " << new_offset << std::endl;
memcpy(&btv, memreg.region + new_offset, sizeof(struct build_tool_version));
std::cout << "BTV tool: " << btv.tool << std::endl;
std::cout << "BTV version: " << btv.version << std::endl;

struct dylinker_command load_link;
key = macroToString[LC_LOAD_DYLINKER];
size = memreg.offsets[key].size;
offset = memreg.offsets[key].offset;
memcpy(&load_link, memreg.region + offset, size);
std::cout << "DYLINKER cmdsize: " << load_link.cmdsize << std::endl;
std::cout << "DYLINKER name: " << load_link.name.offset << std::endl;


std::cout << "Dict: " << std::endl;
    for(std::map<std::string, OffsetAndSize>::iterator iter = memreg.offsets.begin(); iter != memreg.offsets.end(); ++iter) {
        std::string cmd = iter->first;
        std::cout << cmd << std::endl;
    }

    std::vector<std::string> segment_load_commands;
    for(std::map<std::string, OffsetAndSize>::iterator iter = memreg.offsets.begin(); iter != memreg.offsets.end(); ++iter) {
        std::string cmd = iter->first;
        if (cmd.rfind(macroToString[LC_SEGMENT_64], 0) == 0) {
            segment_load_commands.push_back(cmd);
        }
    }

    for(std::string s : segment_load_commands) {
        std::cout << ">>>" << s << std::endl;
    }
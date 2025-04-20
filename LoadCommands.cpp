#include "LoadCommands.h"


LoadCommandHandle::LoadCommandHandle() {}

StringTable::StringTable() {
    this->entries = std::vector<StringTableEntry>();
}

SymbolTable::SymbolTable() {
    this->entries = std::vector<SymbolTableEntry>();
}

SymTabCommandHandle::SymTabCommandHandle() {
    this->load_command = nullptr;
}


void SymTabCommandHandle::print() const {
    std::cout << "        cmd "     <<  macroToString[this->load_command->cmd]  << std::endl;
    std::cout << "    cmdsize "     <<  this->load_command->cmdsize             << std::endl;
    std::cout << "    symoff "      <<  this->load_command->symoff              << std::endl;
    std::cout << "    nsyms "       <<  this->load_command->nsyms               << std::endl;
    std::cout << "    stroff "      <<  this->load_command->stroff              << std::endl;
    std::cout << "    strsize "     <<  this->load_command->strsize             << std::endl;
}

size_t SymbolTable::get_symbol_table_size() {
    return this->entries.size() * sizeof(SymbolTableEntry);
}

////////////////////////////////////////////////////////////////////////////////////////////

SegmentHandle::SegmentHandle() {
    this->load_command = nullptr;
    this->sections = std::vector<SectionWithPayload>();
}

void SegmentHandle::print() const {
    std::cout << "        cmd "            <<  macroToString[this->load_command->cmd]  <<std::endl;
    std::cout << "    cmdsize "                   <<  this->load_command->cmdsize  <<std::endl;
    std::cout << "    segname "           <<  this->load_command->segname  <<std::endl;
    std::cout << "     vmaddr "   <<  this->load_command->vmaddr   <<std::endl;
    std::cout << "     vmsize "   <<  this->load_command->vmsize  <<std::endl;
    std::cout << "    fileoff "                    <<  this->load_command->fileoff  <<std::endl;
    std::cout << "   filesize "                    <<  this->load_command->filesize   <<std::endl;
    std::cout << "    maxprot "           <<  this->load_command->maxprot   <<std::endl;    
    std::cout << "   initprot "           <<  this->load_command->initprot   <<std::endl;    
    std::cout << "     nsects "                    <<  this->load_command->nsects   <<std::endl;    
    std::cout << "      flags "                  <<  this->load_command->flags   <<std::endl;
}


////////////////////////////////////////////////////////////////////////////////////////////

LinkeditDataCommandHandle::LinkeditDataCommandHandle() {
    this->load_command = nullptr;
	this->payload = nullptr;
}

void LinkeditDataCommandHandle::print() const {
}

////////////////////////////////////////////////////////////////////////////////////////////

BuildVersionHandle::BuildVersionHandle() {
    this->load_command = nullptr;
    this->tool_versions = std::vector<struct build_tool_version>();
}

void BuildVersionHandle::print() const {
    std::cout << "ntools: " << this->load_command->ntools << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////////////

OffsetAndSize::OffsetAndSize() {}
OffsetAndSize::OffsetAndSize(uint32_t offset, uint32_t size) : offset(offset), size(size) {}

LoadCommandsRegion::LoadCommandsRegion() {
    this->region = nullptr;
	this->offsets = std::map<std::string, OffsetAndSize>();
}

////////////////////////////////////////////////////////////////////////////////////////////

LoadDylibCommandHandle::LoadDylibCommandHandle() {
    this->load_command = nullptr;
}

void LoadDylibCommandHandle::print() const {
    std::cout << "    cmd "                                     << macroToString[this->load_command->cmd] << std::endl;          
    std::cout << "    cmdsize "                                 << this->load_command->cmd << std::endl;
    std::cout << "       name "                                 << this->library_path_name << " (offset " << this->load_command->dylib.name.offset << ")" << std::endl;
    std::cout << " time stamp 2 Wed Dec 31 21:00:02 1969        " << std::endl;
    std::cout << "    current version 1351.0.0                  " << std::endl;
    std::cout << "compatibility version 1.0.0                   " << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////////////

SourceVersionCommandHandle::SourceVersionCommandHandle() {
    this->load_command = nullptr;
}

void SourceVersionCommandHandle::print() const {
    std::cout << "cmd "      << macroToString[this->load_command->cmd] << std::endl;
    std::cout << "cmdsize "  << this->load_command->cmd << std::endl;
    std::cout << "version "  << this->load_command->version << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////////////

UuidCommandCommandHandle::UuidCommandCommandHandle() {
    this->load_command = nullptr;
}

void UuidCommandCommandHandle::print() const {
    std::cout << "cmd "         << macroToString[this->load_command->cmd] << std::endl;
    std::cout << "cmdsize "     << this->load_command->cmd << std::endl;
    std::cout << "uuid "        << this->load_command->uuid << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////////////

EntryPointCommandHandle::EntryPointCommandHandle() {
    this->load_command = nullptr;
}

void EntryPointCommandHandle::print() const {
    std::cout << "cmd "                 << macroToString[this->load_command->cmd] << std::endl;
    std::cout << "cmdsize "             << this->load_command->cmd << std::endl;
    std::cout << "entryoff "            << this->load_command->entryoff << std::endl;
    std::cout << "stacksize "           << this->load_command->stacksize << std::endl;
}


////////////////////////////////////////////////////////////////////////////////////////////

LoadDyLinkerCommandHandle::LoadDyLinkerCommandHandle() {
    this->load_command = nullptr;
}

void LoadDyLinkerCommandHandle::print() const {
    std::cout << "cmd "                 << macroToString[this->load_command->cmd] << std::endl;
    std::cout << "cmdsize "             << this->load_command->cmd << std::endl;
    std::cout << "name "                << this->pathname << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////////////
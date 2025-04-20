#include "LoadCommands.h"
#include <cmath>
#include <iomanip>

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



DySymTabHandle::DySymTabHandle() {
    this->load_command = nullptr;
}

void DySymTabHandle::print() const {
    std::cout << "    cmd "         << macroToString[this->load_command->cmd] << std::endl;
    std::cout << "    cmdsize "     << this->load_command->cmdsize << std::endl;
    std::cout << "  ilocalsym "     << this->load_command->ilocalsym << std::endl;
    std::cout << "  nlocalsym "     << this->load_command->nlocalsym << std::endl;
    std::cout << " iextdefsym "     << this->load_command->iextdefsym << std::endl;
    std::cout << " nextdefsym "     << this->load_command->nextdefsym << std::endl;
    std::cout << "  iundefsym "     << this->load_command->iundefsym << std::endl;
    std::cout << "  nundefsym "     << this->load_command->nundefsym << std::endl;
    std::cout << "     tocoff "     << this->load_command->tocoff << std::endl;
    std::cout << "       ntoc "     << this->load_command->ntoc << std::endl;
    std::cout << "  modtaboff "     << this->load_command->modtaboff << std::endl;
    std::cout << "    nmodtab "     << this->load_command->nmodtab << std::endl;
    std::cout << "extrefsymoff "    << this->load_command->extrefsymoff << std::endl;
    std::cout << "nextrefsyms "     << this->load_command->nextrefsyms << std::endl;
    std::cout << "indirectsymoff "  << this->load_command->indirectsymoff << std::endl;
    std::cout << "nindirectsyms "   << this->load_command->nindirectsyms << std::endl;
    std::cout << "  extreloff "     << this->load_command->extreloff << std::endl;
    std::cout << "    nextrel "     << this->load_command->nextrel << std::endl;
    std::cout << "  locreloff "     << this->load_command->locreloff << std::endl;
    std::cout << "    nlocrel "     << this->load_command->nlocrel << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////////////


SegmentHandle::SegmentHandle() {
    this->load_command = nullptr;
    this->segname = nullptr;
    this->sections = std::vector<SectionHandle*>();
}

void SegmentHandle::print() const {

    std::cout << "        cmd "            <<  macroToString[this->load_command->cmd]  << std::endl;
    std::cout << "    cmdsize "                   <<  this->load_command->cmdsize  << std::endl;
    std::cout << "    segname "           <<  this->load_command->segname  << std::endl;
    std::cout << "     vmaddr 0x" << std::hex << std::setw(16) << std::setfill('0') << this->load_command->vmaddr << std::endl;
    std::cout << "     vmsize 0x" << std::setw(16) << std::setfill('0') <<  this->load_command->vmsize  << std::endl;
    std::cout << std::dec << "    fileoff "                    <<  this->load_command->fileoff  << std::endl;
    std::cout << "   filesize "                    <<  this->load_command->filesize   << std::endl;
    std::cout << std::hex << "    maxprot 0x"     << std::hex << std::setw(8) << std::setfill('0')  <<  this->load_command->maxprot   << std::endl;    
    std::cout << "   initprot 0x"     << std::hex << std::setw(8) << std::setfill('0')  <<  this->load_command->initprot   << std::endl;    
    std::cout << std::dec <<"     nsects "                    <<  this->load_command->nsects   << std::endl;    
    std::cout << std::hex <<"      flags 0x"                  <<  this->load_command->flags   << std::endl;
    std::cout << std::dec;
    for (SectionHandle* sect_handle : this->sections) {
        sect_handle->print();
    }
    
}

//////////////////////////////////////

SectionHandle::SectionHandle() {
    this->section = nullptr;
    this->payload = nullptr;
}

void SectionHandle::print() const {
    std::cout << "Section"          <<  std::endl;
    std::cout << "  sectname "      <<  this->section->sectname  << std::endl;
    std::cout << "   segname "      <<  this->section->segname  << std::endl;
    std::cout << "      addr 0x" << std::hex << std::setw(16) << std::setfill('0') << this->section->addr  << std::endl;
    std::cout << "      size 0x"      << std::setw(16) << std::setfill('0') << this->section->size  << std::endl;
    std::cout << std::dec << "    offset "      <<  this->section->offset  << std::endl;
    std::cout << "     align 2^"    <<  this->section->align  << " (" << std::pow(2, this->section->align) << ")" << std::endl;
    std::cout << "    reloff "      <<  this->section->reloff  << std::endl;
    std::cout << "    nreloc "      <<  this->section->nreloc  << std::endl;
    std::cout << "     flags 0x"    << std::hex << std::setw(8) << std::setfill('0') << this->section->flags  << std::endl;
    std::cout << std::dec << " reserved1 "      <<  this->section->reserved1  << std::endl;
    std::cout << " reserved2 "      <<  this->section->reserved2 << std::endl;
}


////////////////////////////////////////////////////////////////////////////////////////////

LinkeditDataCommandHandle::LinkeditDataCommandHandle() {
    this->load_command = nullptr;
	this->payload = nullptr;
}

void LinkeditDataCommandHandle::print() const {
    std::cout << " cmd "        << macroToString[this->load_command->cmd] << std::endl;
    std::cout << " cmdsize "    << this->load_command->cmdsize  << std::endl;
    std::cout << " dataoff "    << this->load_command->dataoff  << std::endl;
    std::cout << " datasize "   << this->load_command->datasize  << std::endl; 
}

////////////////////////////////////////////////////////////////////////////////////////////

BuildVersionHandle::BuildVersionHandle() {
    this->load_command = nullptr;
    this->tool_versions = std::vector<struct build_tool_version>();
}

void BuildVersionHandle::print() const {
    std::cout << "    cmd "     << macroToString[this->load_command->cmd]   << std::endl;
    std::cout << "    cmdsize " << this->load_command->cmdsize              << std::endl;
    std::cout << "   platform " << this->load_command->platform             << std::endl;
    std::cout << "      minos " << this->load_command->minos                << std::endl;
    std::cout << "        sdk " << this->load_command->sdk                  << std::endl;
    std::cout << "     ntools " << this->load_command->ntools               << std::endl;

    for (size_t i = 0; i < this->tool_versions.size(); i++) {
        struct build_tool_version btv = this->tool_versions[i];
        std::cout << "        tool " << btv.tool     << std::endl;
        std::cout << "     version " << btv.version  << std::endl;
    }
    
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
    std::cout << "    cmdsize "                                 << this->load_command->cmdsize << std::endl;
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
    std::cout << "cmdsize "  << this->load_command->cmdsize << std::endl;
    std::cout << "version "  << this->load_command->version << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////////////

UuidCommandCommandHandle::UuidCommandCommandHandle() {
    this->load_command = nullptr;
}

void UuidCommandCommandHandle::print() const {
    std::cout << "cmd "         << macroToString[this->load_command->cmd] << std::endl;
    std::cout << "cmdsize "     << this->load_command->cmdsize << std::endl;
    std::cout << "uuid "        << this->load_command->uuid << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////////////

EntryPointCommandHandle::EntryPointCommandHandle() {
    this->load_command = nullptr;
}

void EntryPointCommandHandle::print() const {
    std::cout << "cmd "                 << macroToString[this->load_command->cmd] << std::endl;
    std::cout << "cmdsize "             << this->load_command->cmdsize << std::endl;
    std::cout << "entryoff "            << this->load_command->entryoff << std::endl;
    std::cout << "stacksize "           << this->load_command->stacksize << std::endl;
}


////////////////////////////////////////////////////////////////////////////////////////////

LoadDyLinkerCommandHandle::LoadDyLinkerCommandHandle() {
    this->load_command = nullptr;
}

void LoadDyLinkerCommandHandle::print() const {
    std::cout << "cmd "                 << macroToString[this->load_command->cmd] << std::endl;
    std::cout << "cmdsize "             << this->load_command->cmdsize << std::endl;
    std::cout << "name "                << this->pathname << " (offset " << this->load_command->name.offset << ")" << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////////////
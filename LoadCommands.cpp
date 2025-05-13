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
    this->load_command = new SymTabCommand();
    this->load_command->cmd = LC_SYMTAB;
    this->load_command->cmdsize = sizeof(SymTabCommand);
}


void SymTabCommandHandle::print() const {
    std::cout << "        cmd "     <<  macroToString[this->load_command->cmd]  << std::endl;
    std::cout << "    cmdsize "     <<  this->load_command->cmdsize             << std::endl;
    std::cout << "    symoff "      <<  static_cast<SymTabCommand*>(this->load_command)->symoff              << std::endl;
    std::cout << "    nsyms "       <<  static_cast<SymTabCommand*>(this->load_command)->nsyms               << std::endl;
    std::cout << "    stroff "      <<  static_cast<SymTabCommand*>(this->load_command)->stroff              << std::endl;
    std::cout << "    strsize "     <<  static_cast<SymTabCommand*>(this->load_command)->strsize             << std::endl;
}

size_t SymbolTable::get_symbol_table_size() {
    return this->entries.size() * sizeof(SymbolTableEntry);
}

////////////////////////////////////////////////////////////////////////////////////////////



DySymTabHandle::DySymTabHandle() {
    this->load_command = new DySymTabCommand;
    this->load_command->cmd = LC_DYSYMTAB;
    this->load_command->cmdsize = sizeof(DySymTabCommand);
}

void DySymTabHandle::print() const {
    std::cout << "    cmd "         << macroToString[this->load_command->cmd] << std::endl;
    std::cout << "    cmdsize "     << static_cast<DySymTabCommand*>(this->load_command)->cmdsize << std::endl;
    std::cout << "  ilocalsym "     << static_cast<DySymTabCommand*>(this->load_command)->ilocalsym << std::endl;
    std::cout << "  nlocalsym "     << static_cast<DySymTabCommand*>(this->load_command)->nlocalsym << std::endl;
    std::cout << " iextdefsym "     << static_cast<DySymTabCommand*>(this->load_command)->iextdefsym << std::endl;
    std::cout << " nextdefsym "     << static_cast<DySymTabCommand*>(this->load_command)->nextdefsym << std::endl;
    std::cout << "  iundefsym "     << static_cast<DySymTabCommand*>(this->load_command)->iundefsym << std::endl;
    std::cout << "  nundefsym "     << static_cast<DySymTabCommand*>(this->load_command)->nundefsym << std::endl;
    std::cout << "     tocoff "     << static_cast<DySymTabCommand*>(this->load_command)->tocoff << std::endl;
    std::cout << "       ntoc "     << static_cast<DySymTabCommand*>(this->load_command)->ntoc << std::endl;
    std::cout << "  modtaboff "     << static_cast<DySymTabCommand*>(this->load_command)->modtaboff << std::endl;
    std::cout << "    nmodtab "     << static_cast<DySymTabCommand*>(this->load_command)->nmodtab << std::endl;
    std::cout << "extrefsymoff "    << static_cast<DySymTabCommand*>(this->load_command)->extrefsymoff << std::endl;
    std::cout << "nextrefsyms "     << static_cast<DySymTabCommand*>(this->load_command)->nextrefsyms << std::endl;
    std::cout << "indirectsymoff "  << static_cast<DySymTabCommand*>(this->load_command)->indirectsymoff << std::endl;
    std::cout << "nindirectsyms "   << static_cast<DySymTabCommand*>(this->load_command)->nindirectsyms << std::endl;
    std::cout << "  extreloff "     << static_cast<DySymTabCommand*>(this->load_command)->extreloff << std::endl;
    std::cout << "    nextrel "     << static_cast<DySymTabCommand*>(this->load_command)->nextrel << std::endl;
    std::cout << "  locreloff "     << static_cast<DySymTabCommand*>(this->load_command)->locreloff << std::endl;
    std::cout << "    nlocrel "     << static_cast<DySymTabCommand*>(this->load_command)->nlocrel << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////////////


SegmentHandle::SegmentHandle() {
    this->load_command = new SegmentCommand64();
    this->load_command->cmd = LC_SEGMENT_64;
    this->load_command->cmdsize = sizeof(SegmentCommand64);
    this->segname = {};
    this->sections = std::vector<SectionHandle*>();
}

// TODO: Fix this:
void SegmentHandle::print() const {
/* 
    std::cout << "        cmd "            <<  macroToString[this->load_command->cmd]  << std::endl;
    std::cout << "    cmdsize "           <<  this->load_command->cmdsize  << std::endl;
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
    */
}

//////////////////////////////////////

SectionHandle::SectionHandle() {
    this->section = new Section64();
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
    this->load_command = new LinkeditDataCommand();
	this->payload = nullptr;
}

void LinkeditDataCommandHandle::print() const {
    std::cout << " cmd "        << macroToString[this->load_command->cmd] << std::endl;
    std::cout << " cmdsize "    << this->load_command->cmdsize  << std::endl;
    std::cout << " dataoff "    << this->load_command->dataoff  << std::endl;
    std::cout << " datasize "   << this->load_command->datasize  << std::endl; 
}


DyldChainedFixupsCommandHandle::DyldChainedFixupsCommandHandle() {
    this->load_command = new DyldChainedFixupsCommand();
    this->load_command->cmd = LC_DYLD_CHAINED_FIXUPS;
    this->load_command->cmdsize = sizeof(LinkeditDataCommand);
	this->payload = nullptr;
}

FunctionStartsCommandHandle::FunctionStartsCommandHandle() {
    this->load_command = new FunctionStartsCommand();
    this->load_command->cmd = LC_FUNCTION_STARTS;
    this->load_command->cmdsize = sizeof(LinkeditDataCommand);
	this->payload = nullptr;
}

DataInCodeCommandHandle::DataInCodeCommandHandle() {
    this->load_command = new DataInCodeCommand();
    this->load_command->cmd = LC_DATA_IN_CODE;
    this->load_command->cmdsize = sizeof(LinkeditDataCommand);
	this->payload = nullptr;
}

CodeSignatureCommandHandle::CodeSignatureCommandHandle() {
    this->load_command = new CodeSignatureCommand();
    this->load_command->cmd = LC_CODE_SIGNATURE;
    this->load_command->cmdsize = sizeof(LinkeditDataCommand);
	this->payload = nullptr;
}

ExportsTrieCommandHandle::ExportsTrieCommandHandle() {
    this->load_command = new ExportsTrieCommand();
    this->load_command->cmd = LC_DYLD_EXPORTS_TRIE;
    this->load_command->cmdsize = sizeof(LinkeditDataCommand);
	this->payload = nullptr;
}


////////////////////////////////////////////////////////////////////////////////////////////

BuildVersionHandle::BuildVersionHandle() {
    this->load_command = new BuildVersionCommand();
    this->load_command->cmd = LC_BUILD_VERSION;
    this->tool_versions = std::vector<struct build_tool_version>();
}

void BuildVersionHandle::print() const {
    std::cout << "    cmd "     << macroToString[this->load_command->cmd]   << std::endl;
    std::cout << "    cmdsize " << static_cast<BuildVersionCommand*>(this->load_command)->cmdsize              << std::endl;
    std::cout << "   platform " << static_cast<BuildVersionCommand*>(this->load_command)->platform             << std::endl;
    std::cout << "      minos " << static_cast<BuildVersionCommand*>(this->load_command)->minos                << std::endl;
    std::cout << "        sdk " << static_cast<BuildVersionCommand*>(this->load_command)->sdk                  << std::endl;
    std::cout << "     ntools " << static_cast<BuildVersionCommand*>(this->load_command)->ntools               << std::endl;

    for (size_t i = 0; i < this->tool_versions.size(); i++) {
        struct build_tool_version btv = this->tool_versions[i];
        std::cout << "        tool " << btv.tool     << std::endl;
        std::cout << "     version " << btv.version  << std::endl;
    }
    
}

////////////////////////////////////////////////////////////////////////////////////////////

OffsetAndSize::OffsetAndSize() {}
OffsetAndSize::OffsetAndSize(uint64_t offset, uint64_t size) : offset(offset), size(size) {}

LoadCommandsRegion::LoadCommandsRegion() {
    this->region = nullptr;
	this->offsets = std::map<std::string, OffsetAndSize>();
}

////////////////////////////////////////////////////////////////////////////////////////////

LoadDylibCommandHandle::LoadDylibCommandHandle() {
    this->load_command = new LoadDylibCommand();
    this->load_command->cmd = LC_LOAD_DYLIB;
    size_t lib_system_path_name_size_aligned = alignStringLengthToSixteen(LIB_SYSTEM_PATH_NAME);
    this->load_command->cmdsize = sizeof(LoadDylibCommand) + lib_system_path_name_size_aligned; 
}

void LoadDylibCommandHandle::print() const {
    std::cout << "    cmd "                                     << macroToString[this->load_command->cmd] << std::endl;          
    std::cout << "    cmdsize "                                 << this->load_command->cmdsize << std::endl;
    std::cout << "       name "                                 << this->library_path_name << " (offset " << static_cast<LoadDylibCommand*>(this->load_command)->dylib.name.offset << ")" << std::endl;
    std::cout << " time stamp 2 Wed Dec 31 21:00:02 1969        " << std::endl;
    std::cout << "    current version 1351.0.0                  " << std::endl;
    std::cout << "compatibility version 1.0.0                   " << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////////////

SourceVersionCommandHandle::SourceVersionCommandHandle() {
    this->load_command = new SourceVersionCommand();
    this->load_command->cmd = LC_SOURCE_VERSION;
    this->load_command->cmdsize = sizeof(SourceVersionCommand);
}

void SourceVersionCommandHandle::print() const {
    std::cout << "cmd "      << macroToString[this->load_command->cmd] << std::endl;
    std::cout << "cmdsize "  << this->load_command->cmdsize << std::endl;
    std::cout << "version "  << static_cast<SourceVersionCommand*>(this->load_command)->version << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////////////

UuidCommandHandle::UuidCommandHandle() {
    this->load_command = new UuidCommand();
    this->load_command->cmd = LC_UUID;
    this->load_command->cmdsize = sizeof(UuidCommandHandle);
}

void UuidCommandHandle::print() const {
    std::cout << "cmd "         << macroToString[this->load_command->cmd] << std::endl;
    std::cout << "cmdsize "     << this->load_command->cmdsize << std::endl;
    std::cout << "uuid "        << static_cast<UuidCommand*>(this->load_command)->uuid << std::endl;
}

////////////////////////////////////////////////////////////////////////////////////////////

EntryPointCommandHandle::EntryPointCommandHandle() {
    this->load_command = new EntryPointCommand();
    this->load_command->cmd = LC_MAIN;
    this->load_command->cmdsize = sizeof(EntryPointCommand); 
}

void EntryPointCommandHandle::print() const {
    std::cout << "cmd "                 << macroToString[this->load_command->cmd] << std::endl;
    std::cout << "cmdsize "             << this->load_command->cmdsize << std::endl;
    std::cout << "entryoff "            << static_cast<EntryPointCommand*>(this->load_command)->entryoff << std::endl;
    std::cout << "stacksize "           << static_cast<EntryPointCommand*>(this->load_command)->stacksize << std::endl;
}


////////////////////////////////////////////////////////////////////////////////////////////

LoadDyLinkerCommandHandle::LoadDyLinkerCommandHandle() {
    this->load_command = new LoadDyLinkerCommand();
    this->load_command->cmd = LC_LOAD_DYLINKER;
    size_t dyld_path_name_size_aligned = alignStringLengthToSixteen(DYLD_PATH_NAME);
    this->load_command->cmdsize = sizeof(LoadDyLinkerCommand) + dyld_path_name_size_aligned; 
}

void LoadDyLinkerCommandHandle::print() const {
    std::cout << "cmd "                 << macroToString[this->load_command->cmd] << std::endl;
    std::cout << "cmdsize "             << this->load_command->cmdsize << std::endl;
    std::cout << "name "                << this->pathname << " (offset " << static_cast<LoadDyLinkerCommand*>(this->load_command)->name.offset << ")" << std::endl;
}


////////////////////////////////////////////////////////////////////////////////////////////
#include "ExecutableFileBuilder2.h"

#include "Debugger.h"

#define INITIAL_CAPACITY_BUFFER 64

Buffer::Buffer() {
    this->size = 0;
    this->capacity = INITIAL_CAPACITY_BUFFER;
    this->writing_offset = 0;
    this->data = (Byte*)calloc(this->capacity, sizeof(Byte));
}

uint64_t Buffer::appendData(void* data, uint64_t data_size) {
    uint64_t new_offset = this->writeDataAtOffset(data, data_size, this->writing_offset);
    this->writing_offset += data_size;
    return new_offset;
}

// Both return the 'new' offset.

uint64_t Buffer::writeDataAtOffset(void* data, uint64_t data_size, uint64_t offset) {
    if (this->size + data_size > this->capacity ) {
        this->capacity = 2 * this->capacity;
        this->data = (Byte*)realloc(this->data, this->capacity);
    }
    memcpy(this->data + offset, data, data_size);
    this->size += data_size;
    return this->size;
}

ExecutableFileBuilder2::ExecutableFileBuilder2() {}

ExecutableFileBuilder2::ExecutableFileBuilder2(Macho input_macho) : input_macho(input_macho) {
    this->output_macho = Macho();
    this->page_zero_seg_handle = new SegmentHandle();
    this->buffer = Buffer();
    this->header_offset = 0;
    
}

/*
template <typename E>
void OutputMachHeader<E>::copy_buf(Context<E> &ctx) {
  u8 *buf = ctx.buf + this->hdr.offset;

  std::vector<std::vector<u8>> cmds = create_load_commands(ctx);

  MachHeader &mhdr = *(MachHeader *)buf;
  mhdr.magic = 0xfeedfacf;
  mhdr.cputype = E::cputype;
  mhdr.cpusubtype = E::cpusubtype;
  mhdr.filetype = ctx.output_type;
  mhdr.ncmds = cmds.size();
  mhdr.sizeofcmds = flatten(cmds).size();
  mhdr.flags = MH_TWOLEVEL | MH_NOUNDEFS | MH_DYLDLINK | MH_PIE;


  write_vector(buf + sizeof(mhdr), flatten(cmds));
}
*/


void ExecutableFileBuilder2::buildHeader() {

    this->output_macho.header->magic = MH_MAGIC_64;
    this->output_macho.header->cputype = CPU_TYPE_ARM64;	        
    this->output_macho.header->cpusubtype = 0;	       
    this->output_macho.header->filetype = MH_CORE;	    // <- CHANGEEEEEEE just to test it for now      
    //this->output_macho.header->ncmds = this->output_macho.load_commands.size;		     // TO UPDATE!     
    this->output_macho.header->sizeofcmds = 0;	     // TO UPDATE! 
    this->output_macho.header->flags = MH_NOUNDEFS | MH_DYLDLINK | MH_TWOLEVEL | MH_PIE;     

}

void ExecutableFileBuilder2::patchHeader() {
    this->output_macho.header->filetype = MH_EXECUTE;
}



uint64_t ExecutableFileBuilder2::appendDataToBuffer(void* data, uint64_t data_size) {
    return this->buffer.appendData(data, data_size);
}

uint64_t ExecutableFileBuilder2::writeDataToBufferAtOffset(void* data, uint64_t data_size, uint64_t offset) {
    return this->buffer.writeDataAtOffset(data, data_size, offset);
}

uint64_t ExecutableFileBuilder2::writeHeader(uint64_t offset) {
    return writeDataToBufferAtOffset(this->output_macho.header, sizeof(*this->output_macho.header), this->header_offset);
}


void ExecutableFileBuilder2::buildPageZeroSegmentLoadCommand() {
    this->page_zero_seg_handle->load_command = new SegmentCommand64();
    this->page_zero_seg_handle->segname = SEG_PAGEZERO;

    this->page_zero_seg_handle->load_command->vmaddr	= 0x0000000000000000;
    this->page_zero_seg_handle->load_command->vmsize	= 0x0000000100000000;
    this->page_zero_seg_handle->load_command->fileoff = 0;
    this->page_zero_seg_handle->load_command->filesize = 0;
    this->page_zero_seg_handle->load_command->maxprot = 0;
    this->page_zero_seg_handle->load_command->initprot = 0;
    this->page_zero_seg_handle->load_command->nsects	= 0;
    this->page_zero_seg_handle->load_command->flags = 0 ;
    this->page_zero_seg_handle->load_command->cmd = LC_SEGMENT_64;
    this->page_zero_seg_handle->load_command->cmdsize = sizeof(SegmentCommand64);
}

uint64_t ExecutableFileBuilder2::writePageZeroSegmentLoadCommand(uint64_t offset) {
    SegmentCommand64* load_command = this->page_zero_seg_handle->load_command;
    return writeDataToBufferAtOffset(load_command, load_command->cmdsize, offset);
}

void ExecutableFileBuilder2::patchPageZeroSegmentLoadCommand() {
    this->page_zero_seg_handle->load_command->vmaddr	= 0xFFFFFFFFF;
    this->page_zero_seg_handle->load_command->vmsize	= 0xFFFFFFFFF;
    this->page_zero_seg_handle->load_command->fileoff = 0xFF;
    this->page_zero_seg_handle->load_command->filesize = 0xFF;
    this->page_zero_seg_handle->load_command->maxprot = 0xFF;
    this->page_zero_seg_handle->load_command->initprot = 0xFF;
    this->page_zero_seg_handle->load_command->nsects	= 0xFF;
    this->page_zero_seg_handle->load_command->flags = 0xFF ;
    this->page_zero_seg_handle->load_command->cmd = LC_SEGMENT_64;
    this->page_zero_seg_handle->load_command->cmdsize = sizeof(SegmentCommand64);
}


void ExecutableFileBuilder2::buildExecutableFile() {    
    this->buildHeader();
    this->buildPageZeroSegmentLoadCommand();

    uint64_t writing_offset = 0;
    uint64_t header_offset = 0;

    writing_offset = this->writeHeader(writing_offset);


    uint64_t page_zero_lc_cmd_offset = writing_offset;
    writing_offset = this->writePageZeroSegmentLoadCommand(writing_offset);



    this->patchHeader();

    writing_offset = this->writeHeader(header_offset);

    this->patchPageZeroSegmentLoadCommand();

    this->writePageZeroSegmentLoadCommand(page_zero_lc_cmd_offset);
    
}



void ExecutableFileBuilder2::debug() {
    Debugger d;
    d.dumpRawDataToFile(this->buffer.data,
        0,
        this->buffer.size,
        "BUFFER_DUMP");
}
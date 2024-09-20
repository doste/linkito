#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mach-o/loader.h>

struct mach_header_64* read_macho_header(FILE* fptr)
{
	struct mach_header_64* header = malloc(sizeof(struct mach_header_64));

	size_t read = fread(header, sizeof(struct mach_header_64), 1, fptr);
	
	if (read == 1) {
		return header;
	} else if (ferror(fptr)) {
		fprintf(stderr, "Error while reading the file.\n");
	} else {
		fprintf(stderr, "EOF\n");
	}
	exit(1);
}


void test_headers_are_equal(void) {
	printf("Testing if the headers are equal:\n");
	FILE* fptr_handwritten_exec = fopen("./bin/handwritten_small_exec", "rb");
	if (!fptr_handwritten_exec) {
		fprintf(stderr, "Error while opening the handwritten file.\n");
		exit(1);
	}
	struct mach_header_64* header_handwritten_exec = read_macho_header(fptr_handwritten_exec);
		
	FILE* fptr_from_c_exec = fopen("./bin/small_c_program", "rb");
	if (!fptr_from_c_exec) {
		fprintf(stderr, "Error while opening the from C file.\n");
		exit(1);
	}
	struct mach_header_64* header_from_c_exec = read_macho_header(fptr_from_c_exec);
	
	int equality = memcmp(header_handwritten_exec, header_from_c_exec, sizeof(struct mach_header_64));
	
	if (equality == 0) {
		printf("Test passed.\n");
	} else {
		printf("Test failed.\n");
	}
	
	fclose(fptr_handwritten_exec);
	fclose(fptr_from_c_exec);
}

void get_cmd_and_cmdsize_of_load_command(FILE* fptr_out, uint32_t* cmd, uint32_t* cmdsize) {
	size_t items_read;

	// We'll move the fptr so we need to save its value
	long saved_fptr = ftell(fptr_out);

	// first we get the 'cmd' field
	items_read = fread(cmd, sizeof(uint32_t), 1, fptr_out);
	if (items_read != 1) {
		printf("fread failed.\n");
		exit(1);
	}
	// And then the 'cmdsize' field
	items_read = 0;
	items_read = fread(cmdsize, sizeof(uint32_t), 1, fptr_out);
	if (items_read != 1) {
		printf("fread failed.\n");
		exit(1);
	}

	// Restore the value of the fptr
	if (fseek(fptr_out, saved_fptr, SEEK_SET) != 0) printf("fseek error\n");
}

// Idea to test: Parse the generated mach-o and assert that the cmdsize of each load command is multiple of 8
void test_all_load_cmd_sizes_are_multiple_of_8(void) {
	printf("Testing if all load commands have size multiple of 8:\n");
	
	FILE* fptr_handwritten_exec = fopen("./bin/handwritten_small_exec", "rb");
	if (!fptr_handwritten_exec) {
		fprintf(stderr, "Error while opening the handwritten file.\n");
		exit(1);
	}
	struct mach_header_64 header;
	
	size_t items_read = fread(&header, sizeof(struct mach_header_64), 1, fptr_handwritten_exec);
	if (items_read != 1) {
		printf("fread failed.\n");
		exit(1);
	}
	
	
	// loader.h -> "The load commands directly follow the mach_header[...]"
	// Then we move the fptr foward to skip the header
	//fseek(fptr_handwritten_exec, sizeof(struct mach_header_64), SEEK_SET);   No need
	
	size_t all_load_cmds_ok = 0;
	for (size_t i = 0; i < header.ncmds; i++) {
		uint32_t cmd, cmdsize; 
		get_cmd_and_cmdsize_of_load_command(fptr_handwritten_exec, &cmd, &cmdsize);
		
		if ((cmdsize % 8) != 0) {
			printf("Test failed. (cmd: %d)\n", cmd);
			all_load_cmds_ok = 1;
		}
	}
	if (all_load_cmds_ok == 0) {
		printf("Test passed.\n");
	}
	
	fclose(fptr_handwritten_exec);
}

int main(int argc, char** argv) 
{
	//test_headers_are_equal();
	
	//test_all_load_cmd_sizes_are_multiple_of_8();
	
	return 0;
}
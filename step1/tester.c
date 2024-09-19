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


void test_headers_are_equal()
{
	
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

// Idea to test: Parse the generated mach-o and assert that the cmdsize of each load command is multiple of 8
void test_all_load_cmd_sizes_are_multiple_of_8(void) {
	// TODO
}

int main(int argc, char** argv) 
{
	test_headers_are_equal();
	
	return 0;
}
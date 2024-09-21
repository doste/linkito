all: ../bin/small_c_program.o ../bin/linkito 

../bin/small_c_program.o: small_c_program.c
	cc small_c_program.c -c -o ../bin/small_c_program.o

#../bin/helpers.o: helpers.c
#	cc helpers.c -c -o ../bin/helpers.o

#libhelpers: ../bin/helpers.o
#	ar rcs ../bin/libhelpers.a ../bin/helpers.o

#../bin/macho_parser: macho_parser.c
#	cc macho_parser.c -Wall -o ../bin/macho_parser

../bin/linkito: linkito_step2.c helpers.c debug.c
	cc linkito_step2.c helpers.c macho_parser.c debug.c writer.c builder.c -g -Wall -o ../bin/linkito   #-fuse-ld=mold
	
tester: tester.c
	cc tester.c -o ../bin/test
	
../bin/test: tester
	rm -f ./bin/handwritten_small_exec
	../bin/linkito
	../bin/test
	
clean:
	rm -rf ../bin/*
	
rebuild:
	$(MAKE) clean
	$(MAKE) all
all: small_c_program linkito test

small_c_program: small_c_program.c
	cc small_c_program.c -o ./bin/small_c_program 
	
linkito: linkito_step1.c
	cc linkito_step1.c -Wall -g -o linkito -fuse-ld=mold
	
tester: tester.c
	cc tester.c -o test
	
test: tester
	rm -f ./bin/handwritten_small_exec
	./linkito
	./test
	
clean:
	rm -rf ./bin/*
	rm linkito
	
rebuild:
	$(MAKE) clean
	$(MAKE) all
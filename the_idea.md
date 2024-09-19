**Steps:

1. Create the smallest possible program (i.e. a program with the empty `main`) using the standard macOS dev tools.

2. Write a "linker" that takes no input and produces the file header and the sections as binary blobs which are identical to the smallest executable.

3. Replace binary blobs with code that programmatically generates the same binary blobs one by one, sometimes with some fake data.

4. Once the blobs are eliminated, improve the code so that it can take a single input object file with the empty `main` to produce the same output.

5. Generalize the code gradually so that it can make external function calls, take multiple input files, etc.

6. Add long-tail features one by one so that it can link "real" programs.

The points are 1) you want to keep the program always working and 2) you don't need to understand the entire file format when you begin replacing blobs with code. You'll gradually learn it on the way.

One thing I forget to mention was that I started writing a dumper instead of a producer of an executable file. It worked really well as an exercise to test my understanding of the macOS executable file format.


[ from: https://x.com/rui314/status/1467130631486599168?s=46&t=WzEkQOX3Xc4eRacxvnzZ5Q ]


***Comments:

- Folder step1 contains Step 1 (small_c_program.c) and 2 (linkito_step1.c).



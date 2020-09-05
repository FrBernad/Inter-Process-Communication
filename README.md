# TP1 - Inter Process Communication

# Authors
- [Francisco Bernad](https://github.com/FrBernad)
- [Nicolás Rampoldi](https://github.com/NicolasRampoldi) 

# Program
**SAT solving distribution system** consisting of three files:
  + **solve**: creates slaves and passes parsed data to **vista** process and **output.txt**.
  + **vista**: prints out parsed data to **standar output**.
  + **slave**: parses data using **minisat**.
  
# Compilation

Execute `make` or `make all` command on your shell to compile all source files.
This will create three executable files: 
  + **solve**
  + **vista**
  + **slave**
  
To remove these files, run `make clean` on the directory make was executed
  
# Execution

To run the system first execute the **solve** file by running `./solve` with a list of **.cnf files** as parameters. 

For example: 
```bash
 ./solve files/*
```
The **amount of tasks processed** will be returned by the program through **standard output**. This value can be used by the view program to print the processed files. 

There are two ways of doing this:

1. **Pipe solve's output to view**
   
```bash
./solve files/* | ./view
```
2. **Run files separately**

Run **solve** on **one terminal** and **vista** on **another terminal** passing vista the value returned by solve.
   
```bash
#First terminal:
./solve
54
```

```bash
#Second terminal:
 ./vista 54
```
Run **solve** on **background** and **vista** on **foreground** passing the value returned by solve.
  
```bash
./solve files/*& #background
./vista 54       #foreground
```

Once **solve** finishes, whether **vista** was executed or not, a file named **output.txt** will be created, containing the results.

To remove **output.txt**  and the **executables**, run `make clean`.

# Testing
To test with **PVS-Studio**, **Cppcheck** and **Valgrind** run the following command:
```bash
 make test
```
The tests results can be found in the following files:
 * **PVS-Studio:** *report.tasks*
 * **Valgrind:** *vista.valgrind*, *solve.valgrind*
 * **Cppcheck:** *cppoutput.txt*

To remove these files, run `make cleanTest` on the directory where `make test` was run.
gcc -g -c test.c -I. 
gcc -g -c ../src/gg/gg.c -I. 
gcc -o test test.o gg.o

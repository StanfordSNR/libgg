gcc -c test.c -I ../src/gg
gcc -c ../src/gg/gg.c -I ../src/gg
gcc -o test test.o gg.o

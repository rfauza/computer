cl -c ../test.cpp -o ./test.obj
cl ../main.cpp -L test.obj
./main.exe
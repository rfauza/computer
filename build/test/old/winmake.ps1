rm ./*.obj
rm ./*.exe
clear
cl -c ../basic_components.cpp
cl ../main.cpp -L basic_components.obj
./main.exe
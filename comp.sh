g++ -c pahjong.cpp -std=c++11 -O1 #-g
g++ pahjong.o -o pahjong.elf -lsfml-graphics -lsfml-window -lsfml-system -static-libstdc++ #-pthread
rm pahjong.o

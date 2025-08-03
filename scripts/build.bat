cd ../
mkdir build
cd build
cmake -S .. -B . -G Ninja
cmake --build .
sandbox

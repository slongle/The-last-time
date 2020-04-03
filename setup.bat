rmdir /S /Q build
mkdir build
cd build
cmake ..
::cmake -G "Unix Makefiles" ..
::make
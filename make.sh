if [ ! -d "build" ]; then
    mkdir build
    echo "Created build directory."
else
    echo "Build directory already exists."
fi
cd build
cmake ..
cmake --build . --config Release
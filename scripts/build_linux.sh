
# Jump to root folder
cd ..

# Install deps
sudo apt-get install libstdc++-8-dev clang-7 autotools-dev libtool autoconf xorg-dev libxrandr-dev libxinerama-dev libxcursor-dev libharfbuzz-dev libbz2-dev

# Update submodules to their last commit
git submodule update --init
git submodule foreach "git checkout master"

# Use ImGUI docking branch
cd source/vendor/imgui
git checkout docking
cd ../../..

# Generate GLAD loader
cd source/vendor/glad
python -m glad --generator=c --spec=gl --profile=core --api="gl=4.6" --extensions=../../../scripts/glad_extensions.txt --out-path=.
cd ../../..

# Build deps
mkdir build
cd build
cmake ..
# GLAD
make glad
# GLFW
cd ../source/vendor/glfw
mkdir build
cd build
cmake ..
make
cp src/libglfw3.a ../../../../lib/
cd ../../../../build
# Freetype
cd ../source/vendor/freetype
./autogen.sh
./configure CXXFLAGS=-fPIC CFLAGS=-fPIC LDFLAGS=-fPIC CPPFLAGS=-fPIC
make
cp objs/.libs/libfreetype.a ../../../lib/
cp objs/.libs/libfreetype.so* ../../../lib/
cd ../../../build
# Zlib
cd ../source/vendor/zlib
./configure
make
cp libz.a ../../../lib/
cd ../../../build

# Erwin lib and apps
make erwin -j4
make fudge -j4
make internstr
make sandbox
make fractal

# Initialize intern strings
../bin/internstr


GPU_CMD  = /opt/${UNION_PLATFORM}-toolchain/bin/cmake -DARCH=arm --host=/opt/${UNION_PLATFORM}-toolchain/share/buildroot/toolchainfile.cmake ..

export CROSS=${CROSS_COMPILE}
export CC=${CROSS}gcc
export AR=${CROSS}ar
export AS=${CROSS}as
export LD=${CROSS}ld
export CXX=${CROSS}g++
export HOST=arm-buildroot-linux-gnueabihf

.PHONY: all
all: gpu #sdl2

.PHONY: gpu
gpu:
	cd swiftshader/build && make -j4

.PHONY: clean
clean:
	rm -rf swiftshader/build

.PHONY: cfg
cfg:
	mkdir -p swiftshader/build && cd swiftshader/build && ${GPU_CMD}

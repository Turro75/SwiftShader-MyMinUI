SDL2_CFG+= --disable-oss
SDL2_CFG+= --disable-alsa
SDL2_CFG+= --disable-video-opengl
SDL2_CFG+= --disable-video-opengles
SDL2_CFG+= --disable-video-opengles2
SDL2_CFG+= --enable-audio-mini
SDL2_CFG+= --enable-video-mini

SDL2_INC = -I${PREFIX}/include/SDL2
GPU_CMD  = /opt/${UNION_PLATFORM}-toolchain/bin/cmake -DARCH=arm --host=/opt/${UNION_PLATFORM}-toolchain/share/buildroot/toolchainfile.cmake ..

export CROSS=${CROSS_COMPILE}
export CC=${CROSS}gcc
export AR=${CROSS}ar
export AS=${CROSS}as
export LD=${CROSS}ld
export CXX=${CROSS}g++
export HOST=arm-buildroot-linux-gnueabihf

#$(shell cd sdl2 && ln -s ../swiftshader/build/libEGL.so)
#$(shell cd sdl2 && ln -s ../swiftshader/build/libGLESv2.so)

include Makefile.mk

.PHONY: all
all: gpu #sdl2

.PHONY: gpu
gpu:
	cd swiftshader/build && make -j4

.PHONY: cfg
cfg:
#	cd sdl2 && ./autogen.sh && ./configure ${SDL2_CFG} --host=${HOST}
	mkdir -p swiftshader/build && cd swiftshader/build && ${GPU_CMD}

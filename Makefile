all: ramdisk

ramdisk: main.cpp  
	g++ -w main.cpp -o ramdisk `pkg-config fuse --cflags --libs` -Wall -Werror

clean:
	rm -rf  ramdisk.o ramdisk

#this will have to change when we add the TinyFS.c interface
tinyFsDemo : tinyFsDemo.c libDisk.c libTinyFS.c linkedList.c
	gcc -Wall -Werror -o $@ *.c -I.

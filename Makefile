#Sample Makefile

CFLAGS=-g -O0

all:	check

default: check

clean-ckpt:
	rm -rf myckpt

clean: clean-ckpt
	rm -rf myrestart hello ckpt ckpt.o libckpt.so hello.o

ckpt.o: ckpt.c
	gcc ${CFLAGS} -fno-stack-check -fno-stack-protector -c -Wall -Werror -fpic -o ckpt.o ckpt.c

libckpt.so: ckpt.o
	gcc ${CFLAGS} -fno-stack-check -fno-stack-protector -shared -o libckpt.so ckpt.o

hello.o: hello.c
	gcc ${CFLAGS} -fno-stack-check -fno-stack-protector -c -Wall -Werror -fpic -o hello.o hello.c

hello:	hello.o
	gcc ${CFLAGS} -fno-stack-check -fno-stack-protector -g -o hello hello.o

restart: myrestart.c
	gcc ${CFLAGS} -fno-stack-check -fno-stack-protector -g -static -Wl,-Ttext-segment=5000000 -Wl,-Tdata=5100000 -Wl,-Tbss=5200000 -o myrestart myrestart.c

res: 	restart
	./myrestart myckpt

gdb:
	gdb --args ./myrestart myckpt

check:	clean-ckpt libckpt.so hello myrestart
	(sleep 3 && kill -12 `pgrep -n hello` && sleep 2 && pkill -9 hello) &
	LD_PRELOAD=`pwd`/libckpt.so ./hello
	(sleep 2 &&  pkill -9 myrestart) & make res

dist:
	dir=`basename $$PWD`; cd ..; tar cvf $$dir.tar ./$$dir; gzip $$dir.tar

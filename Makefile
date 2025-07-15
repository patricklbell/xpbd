xpdb: *.c
	g++ -g -pthread main.c -lX11 -lXext -DWAYLAND -o ./build/xpbd
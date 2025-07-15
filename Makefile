xpdb: *.c
	g++ -g main.c -lX11 -lXext -DWAYLAND -o ./build/xpbd
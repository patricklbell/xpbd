xpdb: **/*.c
	g++ -g xpbd/main.c -lX11 -lXext -DWAYLAND -o ./build/xpbd
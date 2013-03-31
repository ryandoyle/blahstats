all:
	gcc -lm -lpthread stats.c -o stats

clean:
	rm stats


all:
	gcc stats.c -lm -lpthread -o stats

clean:
	rm stats


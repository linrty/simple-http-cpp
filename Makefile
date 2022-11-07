CC=g++
CFLAGS=-ggdb -Wall -Wextra -pedantic

httpd: tpool.o httpd.o ban.o main.o plugin.o timer.o
	$(CC) tpool.o httpd.o ban.o main.o plugin.o timer.o $(CFLAGS) -lpthread -o httpd

tpool.o: tpool.cpp
	$(CC) tpool.cpp -c $(CFLAGS) -o tpool.o

httpd.o: httpd.cpp
	$(CC) httpd.cpp -c $(CFLAGS) -lpthread -o httpd.o

ban.o: ban.cpp
	$(CC) ban.cpp -c $(CFLAGS) -o ban.o

plugin.o: plugin.cpp
	$(CC) plugin.cpp -c $(CFLAGS) -o plugin.o

timer.o: timer.cpp
	$(CC) timer.cpp -c $(CFLAGS) -o timer.o

threadpool.o: threadpool.cpp
	$(CC) threadpool.cpp -c $(CFLAGS) -o threadpool.o

main.o: main.cpp
	$(CC) main.cpp -c $(CFLAGS) -o main.o

clean:
	rm -rf *.o

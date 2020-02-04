all: a.out


a.out: main.o bcm2835.o
	g++ $^ -o $@

clean:
	rm -f *.o
	rm -f a.out


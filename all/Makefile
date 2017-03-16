LDFLAGS = -lm -lmraa -lpthread -lssl -lcrypto
SRC = lab4_1.c lab4_2.c lab4_3.c lcd.c lcd.h lab4_1.log lab4_2.log lab4_3.log README disp.png Makefile
OBJ = lab4_1 lab4_2 lab4_3 lab4_1.o lab4_2.o lab4_3.o lcd.o lab4_1.log lab4_2.log lab4_3.log lab4-804608241.tar.gz

all: lab4_1 lab4_2 lab4_3

lab4_1: lab4_1.c
	gcc -o lab4_1 lab4_1.c $(LDFLAGS)

lab4_2: lab4_2.o lcd.o
	gcc -o lab4_2 lab4_2.o lcd.o $(LDFLAGS)

lab4_3: lab4_3.o lcd.o
	gcc -o lab4_3 lab4_3.o lcd.o $(LDFLAGS)

lab4_2.o: lab4_2.c
	gcc -o lab4_2.o -c lab4_2.c

lab4_3.o: lab4_3.c
	gcc -o lab4_3.o -c lab4_3.c

lcd.o: lcd.c lcd.h
	gcc -o lcd.o -c lcd.c

dist: $(SRC)
	tar -czvf lab4-804608241.tar.gz $(SRC)

clean:
	rm -f $(OBJ)

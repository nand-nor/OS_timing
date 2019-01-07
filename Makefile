#Allie Clifford
#Makefile for comp111 assignment 1
#Date created: 9/16/2018
#Last modified: 9/16/2018

CC = gcc
CLFAGS = -g -std=c99 -Wall -Wextra -Werror -Wfatal-errors -pedantic
LDLIBS = -lm -lrt 

all: a1

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

a1: timing.o
	$(CC) $^ -o $@ $(LDLIBS)

clean:
	rm -f a1 *.o


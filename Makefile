CFLAGS=-std=c99 -Wall -pedantic -g -DCOMMENT=0

all:
	clang irc.c bot.c -o bot

clean:
	rm -rf bot

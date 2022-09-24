all: cshell.c
	gcc -o cshell cshell.c

clean:
	$(RM) cshell
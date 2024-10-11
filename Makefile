# Filename: Makefile
# Author: 
# Date: 2-10-2024

sources = src/*.c

main: $(sources)
	gcc $^ -o main

clean:
	rm -f main

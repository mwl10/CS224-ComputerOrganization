#
# Student makefile for Cache Lab
# 
CC = gcc
CFLAGS = -g -Wall -Werror -std=c99 -Og -Wno-unused-value

all: csim 

csim: csim.c cachelab.c cachelab.h
	$(CC) $(CFLAGS) -o csim csim.c cachelab.c -lm 

submit:
	-cp csim.c cachelab-submit/csim.c 
	submit224 cachelab-submit cachelab-submit

#
# Clean the src dirctory
#
clean:
	rm -rf *.o csim.out csim-ref.out
	rm -f csim
	rm -f .csim_results .marker 

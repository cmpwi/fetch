
CFLAGS = -std=c23 -Wall -Werror -O3

SRCS = fetch.c
OBJS = ${SRCS:S/.c/.o/g}
OBJDIR = obj
TARGET = fetch

all: fetch

obj:
	mkdir -p ${OBJDIR}

.c.o:
	${CC} ${CFLAGS} -c ${.IMPSRC} -o ${.TARGET}

fetch: ${OBJS}
	${CC} -o ${.TARGET} ${OBJS}

.PHONY: all

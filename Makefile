
CFLAGS = -std=c23 -Wall -Werror -Os

SRCS = fetch.c
OBJS = ${SRCS:S/.c/.o/g}
OBJDIR = obj
TARGET = fetch

all: ${TARGET}

obj:
	mkdir -p ${OBJDIR}

.c.o:
	${CC} ${CFLAGS} -c ${.IMPSRC} -o ${.TARGET}

${TARGET}: ${OBJS}
	${CC} -o ${.TARGET} ${OBJS}

.PHONY: all

CFLAGS += -MMD
SRCS = main.c

.PHONY: clean depclean

all: rfc-list

rfc-list: $(SRCS:.c=.o)
	$(CC) -o $@ $(SRCS:.c=.o)

clean: depclean
	- rm -f $(SRCS:.c=.o)

depclean:
	- rm -f $(SRCS:.c=.d)

-include $(SRCS:.c=.d)

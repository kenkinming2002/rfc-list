DESTDIR ?= /usr/local

CPPFLAGS += -MMD

CFLAGS += $(shell pkg-config --cflags libcurl)
LIBS += $(shell pkg-config --libs libcurl)

SRCS = main.c spawn.c

.PHONY: clean depclean install

all: rfc-list

rfc-list: $(SRCS:.c=.o)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRCS:.c=.o) $(LIBS)

clean: depclean
	- rm -f $(SRCS:.c=.o)

depclean:
	- rm -f $(SRCS:.c=.d)

install: rfc-list
	install -m 0755 rfc-list $(DESTDIR)/bin

-include $(SRCS:.c=.d)

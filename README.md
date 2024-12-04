# rfc-list
Simple command line program that allows searching for and opening RFC documents
through fzf.

This is inspired by emacs rfc-mode. Because I am vim user and vim is not an
operating system, I choose to implement this as a simple command line program.

## Building and installation
```sh
$ make -j
$ make install
```

## Caveat
Because we are fetching the rfc index document every time the program is used,
there is a significant latency. A better and much simpler approach is probably
to just download all rfc documents in advance and grep, find, fzf, ... the
downloaded documents instead.

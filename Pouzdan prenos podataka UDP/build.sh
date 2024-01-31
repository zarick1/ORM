#!/bin/sh

set -ex

gcc -W -o receiver	receiver.c	-lncurses
gcc -W -o sender	sender.c	-lncurses

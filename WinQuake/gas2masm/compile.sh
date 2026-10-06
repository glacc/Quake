#!/bin/bash

SWD="$(dirname "$(realpath "$0")")"

gcc "$SWD/gas2masm.c" -O0 -g -o "$SWD/gas2masm"

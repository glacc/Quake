#!/bin/bash

SWD="$(dirname "$(realpath "$0")")"

"$SWD/gen_dos.py" > "$SWD/compile_dos.bat"

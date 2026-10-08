#!/bin/python

# ./gen_compile_dos.py > compile_dos.bat
# compile_dos.bat > compile_dos.log 2>&1

import os

output_name = "QUAKE_N.EXE"

cflags = "-fcommon -fexpensive-optimizations -O3"
# cflags = "-fcommon -g -O0"

cc = "gcc -std=gnu90 -trigraphs " + cflags + " -c"
ac = "gcc -x assembler-with-cpp " + cflags + " -c"
link = "gcc -lm -v -s"

files = os.listdir("./")
files.sort()

c_files_exclude = {
    "cd_null.c", "cd_linux.c", "cd_win.c",
    
    "in_null.c", "in_sun.c", "in_win.c",
    
    "snd_linux.c", "snd_next.c", "snd_sun.c", "snd_win.c", "snd_null.c",
    # "snd_dos.c",
    
    "sys_linux.c", "sys_null.c", "sys_sun.c", "sys_win.c", "sys_wind.c",
    
    "vid_null.c", "vid_svgalib.c", "vid_sunx.c", "vid_sunxil.c", "vid_win.c", "vid_x.c",

    "conproc.c",

    "mplpc.c",
}

s_files_exclude = {
    "sys_wina.s"
}

def is_excluded_c(filename: str):
    if filename.startswith("gl_"):
        return True
    if filename.startswith("net_") and filename != "net_none.c" and filename != "net_main.c" and filename != "net_loop.c" and filename != "net_main.c" and filename != "net_vcr.c":
        return True
    if filename in c_files_exclude:
        return True
    return False

def is_excluded_s(filename: str):
    if filename in s_files_exclude:
        return True
    return False

link_list = ""

for file in files:
    filename, extension = os.path.splitext(file)
    if extension == ".c" and not is_excluded_c(file):
        print(cc, file)
        link_list += filename + ".o "

for file in files:
    filename, extension = os.path.splitext(file)
    if extension == ".s" and not is_excluded_s(file):
        print(ac, file)
        link_list += filename + ".o "

# print(link, link_list, "-o", output_name)
print(link, "*.o", "-o" , output_name)


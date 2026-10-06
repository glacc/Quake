#!/bin/python

import os

output_name = "Quake.exe"

cc = "gcc -std=gnu90 -trigraphs -c"
ac = "gcc -x assembler-with-cpp -c"
link = "gcc"

files = os.listdir("./")

c_files_exclude = {
    "cd_audio.c", "cd_linux.c", "cd_win.c",
    "conproc.c",
    "in_null.c", "in_sun.c", "in_win.c",
    "nonintel.c",
    "snd_linux.c", "snd_sun.c", "snd_win.c",
    "sys_linux.c", "sys_sun.c", "sys_win.c", "sys_wind.c",
    "vid_null.c", "vid_sunx.c", "vid_sunxil.c", "vid_win.c", "vid_x.c",

    # "d_part.c", "d_polys.c", "d_scan.c", "d_vars.c",
    # "r_aclip.c", "r_alias.c", "r_draw.c", "r_edge.c", "r_vars.c"
}

s_files_exclude = {
    "sys_wina.s"
}

def is_excluded_c(filename: str):
    if filename.startswith("gl_"):
        return True
    if filename.startswith("net_") and filename != "net_none.c":
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

print(link, link_list, "-o", output_name)


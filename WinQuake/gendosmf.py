#!/bin/python

import os
import sys

build_dir = "builddos"

output_name = "QUAKE_N.EXE"

cc = "gcc"

cflags_all = "-fcommon"
ldflags_lib = "-lm"

cflags_cc_release = "-fexpensive-optimizations -O3"
cflags_cc_debug   = "-g -O0"
cflags_as_release = ""
cflags_as_debug   = ""
ldflags_release = "-s"
ldflags_debug   = ""

cflags_cc = cflags_cc_release
cflags_as = cflags_as_release
ldflags_extra = ldflags_release

no_asm = False

for arg in sys.argv[1:]:
    match arg:
        case "release":
            pass
        case "debug":
            cflags_cc = cflags_cc_debug
            cflags_as = cflags_as_debug
            ldflags_extra = ldflags_debug
        case "noasm":
            no_asm = True

do_cc = "$(CC) -std=gnu90 -trigraphs -MMD $(CFLAGS_ALL) $(CFLAGS_CC) $(CFLAGS_INC) -c $< -o $@"
do_as = "$(CC) -x assembler-with-cpp -MMD $(CFLAGS_ALL) $(CFLAGS_AS) $(CFLAGS_INC) -c $< -o $@"
link = "$(CC) $(LD_LIBS) $(LDFLAGS) -o $@"

dirs = {
    "",
    "pr_32bit",
}

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

if no_asm:
    # s_files_exclude.add("sys_dosa.s")
    cflags_all += " -D_DISABLE_ID386"

def is_excluded_c(filename: str):
    if filename.startswith("gl_") or filename.startswith("sdl_") or filename.endswith("_sdl.c"):
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

def file_with_path(dir: str, filename: str, extension: str):
    return dir + "/" + filename + extension

cflags_inc = ""

objs = ""
obj_rules = ""

for dir in dirs:
    dir_to_list = dir
    if dir == "":
        dir_to_list = "."
    files = os.listdir(dir_to_list)
    files.sort()

    cflags_inc += "-I$(CURDIR)/" + dir + " "

    objdir = build_dir
    if dir != "":
        objdir += "/" + dir

    for file in files:
        filename, extension = os.path.splitext(file)
        filepath = file_with_path(dir_to_list, filename, extension)
        if extension == ".c" and not is_excluded_c(file):
            objpath = file_with_path(objdir, filename, ".o")
            obj_rules += objpath + " : " + filepath + "\n\t$(DO_CC)\n\n"
            objs += " \\\n\t" + objpath

    for file in files:
        filename, extension = os.path.splitext(file)
        filepath = file_with_path(dir_to_list, filename, extension)
        if extension == ".s" and not is_excluded_s(file):
            objpath = file_with_path(objdir, filename, ".o")
            obj_rules += objpath + " : " + filepath + "\n\t$(DO_AS)\n\n"
            objs += " \\\n\t" + objpath

with open("mkfdos", "w") as makefile:
    outfile = file_with_path(build_dir, output_name, "")

    print("CC=" + cc + "\n", file=makefile)

    print("CFLAGS_ALL=" + cflags_all, file=makefile)
    print("CFLAGS_INC=" + cflags_inc.strip(), file=makefile)
    print("CFLAGS_CC=" + cflags_cc, file=makefile)
    print("CFLAGS_AS=" + cflags_as, file=makefile)
    print("LD_LIBS=" + ldflags_lib, file=makefile)
    print("LDFLAGS=" + ldflags_extra + "\n", file=makefile)

    print("DO_CC=" + do_cc, file=makefile)
    print("DO_AS=" + do_as, file=makefile)
    print("LD=" + link + "\n", file=makefile)

    print("release : mkdir", outfile + "\n", file=makefile)

    print("mkdir :", file=makefile)
    for dir in dirs:
        dirpath = ("\"" + build_dir + "/" + dir + "\"").replace("/", "\\")
        print("\tcmd /c if not exist " + dirpath + " mkdir " + dirpath, file=makefile)
    print("\n", file=makefile)

    print("OBJS=" + objs + "\n", file=makefile)

    print("-include " + file_with_path(build_dir, "*", "*.d") + "\n", file=makefile)

    print(obj_rules, file=makefile)

    print(outfile + " : $(OBJS)\n\t$(LD) $(OBJS)\n", file=makefile)

    print("clean :", file=makefile)
    for dir in dirs:
        subdir = ""
        if dir != "":
            subdir = "\\" + dir
        dirpath = (build_dir + subdir).replace("/", "\\")
        print("\tcmd /c del \"" + dirpath + "\\*.o\"\n\tcmd /c del \"" + dirpath + "\\*.d\"", file=makefile)

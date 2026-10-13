#!/bin/python

import os
import sys

build_dir = "buildsdl"

output_name = "QUAKESDL.ELF"

cc = "gcc"

cflags_all = "-fcommon -D_SDL"# -DMAXWIDTH=2048 -DMAXHEIGHT=2048"
ldflags_lib = "-lm -lSDL3 -lvorbisfile"

cflags_cc_release = "-fexpensive-optimizations -O3"
cflags_cc_debug   = "-g -O0"
cflags_as_release = ""
cflags_as_debug   = ""
ldflags_release = "-s"
ldflags_debug   = ""

cflags_cc = cflags_cc_release
cflags_as = cflags_as_release
ldflags_extra = ldflags_release

build_32bit = False

for arg in sys.argv[1:]:
    match arg:
        case "release":
            pass
        case "debug":
            cflags_cc = cflags_cc_debug
            cflags_as = cflags_as_debug
            ldflags_extra = ldflags_debug
        case "-m32":
            cflags_all += " -m32 -march=pentiumpro"
            ldflags_extra += " -m32"
            build_32bit = True

do_cc = "$(CC) -std=gnu90 -trigraphs -MMD $(CFLAGS_ALL) $(CFLAGS_CC) $(CFLAGS_INC) -c $< -o $@"
do_as = "$(CC) -x assembler-with-cpp -DELF -MMD $(CFLAGS_ALL) $(CFLAGS_AS) $(CFLAGS_INC) -c $< -o $@"
link = "$(CC) $(LD_LIBS) $(LDFLAGS) -o $@"

dirs = {
    "",
}

c_files_exclude = {
    "dos_v2.c",
    "mplib.c",
    "conproc.c",
    "mplpc.c",
    "vregset.c"
}

c_files_not_exc = {
    "cd_sdl.c",
    "snd_sdl.c",
    "snd_mem_sdl.c",
    "sys_sdl.c",
    "vid_sdl.c",
    "in_sdl.c",

    "vid_wide.c",

    "net_none.c",
    "net_main.c",
    "net_loop.c",
    "net_vcr.c",
}

s_files_exclude = {
    "sys_dosa.s",
    "d_copy.s",
    "snd_mixa.s",
}

if build_32bit:
    dirs.add("pr_32bit")
else:
    dirs.add("pr_64fix")
    # dirs.add("pr_32bit")
    s_files_exclude.add("sys_wina.s")

def is_excluded_c(filename: str):
    if filename in c_files_not_exc:
        return False
    if filename.startswith("gl_") or filename.startswith("cd_") or filename.startswith("snd_") or filename.startswith("in_") or filename.startswith("sys_") or filename.startswith("vid_") or filename.startswith("net_"):
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

with open("mkfsdl", "w") as makefile:
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

    print("default : mkdir", outfile + "\n", file=makefile)

    print("mkdir :", file=makefile)
    for dir in dirs:
        dirpath = "\"" + build_dir + "/" + dir + "\""
        print("\tmkdir -p " + dirpath, file=makefile)
    print("\n", file=makefile)

    print("OBJS=" + objs + "\n", file=makefile)

    print("-include " + file_with_path(build_dir, "*", "*.d") + "\n", file=makefile)

    print(obj_rules, file=makefile)

    print(outfile + " : $(OBJS)\n\t$(LD) $(OBJS)\n", file=makefile)

    print("clean :", file=makefile)
    for dir in dirs:
        subdir = ""
        if dir != "":
            subdir = "/" + dir
        dirpath = build_dir + subdir
        print("\trm -f \"" + dirpath + "/\"*.o\n\trm -f \"" + dirpath + "/\"*.d", file=makefile)

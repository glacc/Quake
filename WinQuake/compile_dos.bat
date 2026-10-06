gcc -x assembler-with-cpp -c d_scana.s
gcc -x assembler-with-cpp -c worlda.s
gcc -std=gnu90 -c vregset.c
gcc -x assembler-with-cpp -c d_draw.s
gcc -std=gnu90 -c vid_vga.c
gcc -std=gnu90 -c sys_dos.c
gcc -std=gnu90 -c model.c
gcc -std=gnu90 -c keys.c
gcc -std=gnu90 -c pr_exec.c
gcc -std=gnu90 -c vid_svgalib.c
gcc -std=gnu90 -c d_edge.c
gcc -std=gnu90 -c d_scan.c
gcc -std=gnu90 -c d_part.c
gcc -std=gnu90 -c snd_mix.c
gcc -std=gnu90 -c r_bsp.c
gcc -x assembler-with-cpp -c d_parta.s
gcc -std=gnu90 -c d_modech.c
gcc -std=gnu90 -c r_edge.c
gcc -std=gnu90 -c sys_null.c
gcc -std=gnu90 -c cl_tent.c
gcc -std=gnu90 -c d_init.c
gcc -x assembler-with-cpp -c d_polysa.s
gcc -std=gnu90 -c mplpc.c
gcc -x assembler-with-cpp -c surf16.s
gcc -std=gnu90 -c conproc.c
gcc -std=gnu90 -c cl_input.c
gcc -x assembler-with-cpp -c surf8.s
gcc -std=gnu90 -c cvar.c
gcc -x assembler-with-cpp -c dosasm.s
gcc -std=gnu90 -c console.c
gcc -std=gnu90 -c d_sprite.c
gcc -std=gnu90 -c d_fill.c
gcc -std=gnu90 -c sv_move.c
gcc -x assembler-with-cpp -c r_aclipa.s
gcc -x assembler-with-cpp -c sys_dosa.s
gcc -std=gnu90 -c pr_cmds.c
gcc -std=gnu90 -c snd_dos.c
gcc -std=gnu90 -c sv_user.c
gcc -std=gnu90 -c d_polyse.c
gcc -std=gnu90 -c zone.c
gcc -std=gnu90 -c d_zpoint.c
gcc -x assembler-with-cpp -c d_varsa.s
gcc -std=gnu90 -c r_alias.c
gcc -std=gnu90 -c common.c
gcc -std=gnu90 -c r_light.c
gcc -x assembler-with-cpp -c r_drawa.s
gcc -std=gnu90 -c r_sky.c
gcc -std=gnu90 -c cl_demo.c
gcc -std=gnu90 -c pr_edict.c
gcc -std=gnu90 -c snd_null.c
gcc -std=gnu90 -c cl_parse.c
gcc -std=gnu90 -c d_surf.c
gcc -std=gnu90 -c cd_null.c
gcc -std=gnu90 -c sys_wind.c
gcc -std=gnu90 -c d_vars.c
gcc -std=gnu90 -c r_vars.c
gcc -std=gnu90 -c r_efrag.c
gcc -std=gnu90 -c host.c
gcc -std=gnu90 -c in_dos.c
gcc -std=gnu90 -c snd_gus.c
gcc -std=gnu90 -c snd_next.c
gcc -std=gnu90 -c snd_dma.c
gcc -std=gnu90 -c r_main.c
gcc -std=gnu90 -c sv_main.c
gcc -std=gnu90 -c vid_dos.c
gcc -x assembler-with-cpp -c r_edgea.s
gcc -x assembler-with-cpp -c d_draw16.s
gcc -std=gnu90 -c dos_v2.c
gcc -std=gnu90 -c sbar.c
gcc -x assembler-with-cpp -c snd_mixa.s
gcc -std=gnu90 -c net_none.c
gcc -std=gnu90 -c r_surf.c
gcc -std=gnu90 -c r_sprite.c
gcc -x assembler-with-cpp -c r_aliasa.s
gcc -std=gnu90 -c cl_main.c
gcc -x assembler-with-cpp -c r_varsa.s
gcc -std=gnu90 -c host_cmd.c
gcc -std=gnu90 -c r_misc.c
gcc -std=gnu90 -c d_sky.c
gcc -std=gnu90 -c screen.c
gcc -std=gnu90 -c menu.c
gcc -std=gnu90 -c draw.c
gcc -std=gnu90 -c r_draw.c
gcc -std=gnu90 -c wad.c
gcc -std=gnu90 -c r_aclip.c
gcc -std=gnu90 -c world.c
gcc -std=gnu90 -c snd_mem.c
gcc -std=gnu90 -c crc.c
gcc -std=gnu90 -c sv_phys.c
gcc -std=gnu90 -c vid_ext.c
gcc -x assembler-with-cpp -c d_copy.s
gcc -std=gnu90 -c mathlib.c
gcc -std=gnu90 -c chase.c
gcc -x assembler-with-cpp -c math.s
gcc -x assembler-with-cpp -c d_spr8.s
gcc -std=gnu90 -c mplib.c
gcc -std=gnu90 -c view.c
gcc -std=gnu90 -c cmd.c
gcc -std=gnu90 -c r_part.c
gcc d_scana.o worlda.o vregset.o d_draw.o vid_vga.o sys_dos.o model.o keys.o pr_exec.o vid_svgalib.o d_edge.o d_scan.o d_part.o snd_mix.o r_bsp.o d_parta.o d_modech.o r_edge.o sys_null.o cl_tent.o d_init.o d_polysa.o mplpc.o surf16.o conproc.o cl_input.o surf8.o cvar.o dosasm.o console.o d_sprite.o d_fill.o sv_move.o r_aclipa.o sys_dosa.o pr_cmds.o snd_dos.o sv_user.o d_polyse.o zone.o d_zpoint.o d_varsa.o r_alias.o common.o r_light.o r_drawa.o r_sky.o cl_demo.o pr_edict.o snd_null.o cl_parse.o d_surf.o cd_null.o sys_wind.o d_vars.o r_vars.o r_efrag.o host.o in_dos.o snd_gus.o snd_next.o snd_dma.o r_main.o sv_main.o vid_dos.o r_edgea.o d_draw16.o dos_v2.o sbar.o snd_mixa.o net_none.o r_surf.o r_sprite.o r_aliasa.o cl_main.o r_varsa.o host_cmd.o r_misc.o d_sky.o screen.o menu.o draw.o r_draw.o wad.o r_aclip.o world.o snd_mem.o crc.o sv_phys.o vid_ext.o d_copy.o mathlib.o chase.o math.o d_spr8.o mplib.o view.o cmd.o r_part.o  -o Quake.exe

############################################################################
# SMMU Makefile

MODE = RELEASE
O = $(O_$(MODE))
O_RELEASE = obj
O_DEBUG = objdebug
DEPENDENCIES = 1

# 15/10/99 sf: added multi-platform support

# select platform here:
DJGPPDOS=1
# LINUX=1

################################ DJGPP ####################################

ifdef DJGPPDOS

PLATFORM = djgpp
        
# compiler
CC=gcc
        
# the command you use to delete files
RM=del
        
# the command you use to copy files
CP=copy /y
        
# the exe file name -sf
EXE=$(O)/smmu.exe
        
# options common to all builds
CFLAGS_COMMON=-Wall -g

# new features; comment out what you don't want at the moment
CFLAGS_NEWFEATURES=-DDOGS
        
# debug options
CFLAGS_DEBUG=-g -O2 -DRANGECHECK -DINSTRUMENTED
LDFLAGS_DEBUG=
        
# optimized (release) options
CFLAGS_RELEASE=-O3 -ffast-math -fomit-frame-pointer -m486 -mreg-alloc=adcbSDB
LDFLAGS_RELEASE=
# -s
       
# libraries to link in
LIBS=-lalleg -lm -lemu
        
# this selects flags based on debug and release tagets
CFLAGS=$(CFLAGS_COMMON)  $(CFLAGS_$(MODE)) $(CFLAGS_NEWFEATURES)
LDFLAGS=$(LDFLAGS_COMMON) $(LDFLAGS_$(MODE))
        
# system-specific object files
PLATOBJS =                  \
	$(O)/i_main.o       \
	$(O)/i_system.o     \
	$(O)/i_sound.o      \
	$(O)/i_video.o      \
	$(O)/i_net.o        \
	$(O)/ser_main.o     \
	$(O)/ser_port.o     \
	$(O)/keyboard.o     \
	$(O)/mmus2mid.o     \
	$(O)/pproblit.o     \
	$(O)/drawspan.o     \
	$(O)/emu8kmid.o     \
	$(O)/drawcol.o

build : $(EXE)

$(EXE): $(OBJS) $(O)/version.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(O)/version.o -o $@ $(LIBS)

release: sources binaries

sources:
# build source zip
	make clean
	$(RM) smmu-src.zip
	pkzip -a -ex -rp smmu-src

binaries:
# build binaries zip
	del $(O)\smmu.exe
	make build
	strip $(O)/smmu.exe
	djp $(O)/smmu.exe
	if exist smmu.zip del smmu.zip
	pkzip -a -ex smmu $(O)/smmu.exe $(O)/smmu.wad               \
			smmu.txt smmuedit.txt                       \
			doomlic.txt copying copying.dj 
	pkzip -a -ex -rp smmu examples/*.wad

clean:
	del *.c~
	del *.h~
	del $(O)\tranmap.dat
	del $(O)\smmu.cfg
	if exist $(O_RELEASE)\*.exe del $(O_RELEASE)\*.exe
	if exist $(O_DEBUG)\*.exe del $(O_DEBUG)\*.exe
	if exist $(O_RELEASE)\*.o del $(O_RELEASE)\*.o
	if exist $(O_DEBUG)\*.o del $(O_DEBUG)\*.o

endif

################################# LINUX #####################################

# nb. linux support not yet finished :(

ifdef LINUX

PLATFORM = linux
# compiler
CC = gcc
        
# the command you use to delete files
RM = rm
# the command you use to copy files
CP = cp
                
# options common to all builds
CFLAGS_COMMON = -Wall -g
        
# new features; comment out what you don't want at the moment
CFLAGS_NEWFEATURES = -DDOGS
        
# debug options
CFLAGS_DEBUG = -g -O2 -DRANGECHECK -DINSTRUMENTED
LDFLAGS_DEBUG =
        
# optimized (release) options
CFLAGS_RELEASE = -O2
LDFLAGS_RELEASE = 
# -s
        
# this selects flags based on debug and release tagets
CFLAGS =  $(CFLAGS_COMMON)  $(CFLAGS_$(MODE)) $(CFLAGS_NEWFEATURES)
LDFLAGS = $(LDFLAGS_COMMON) $(LDFLAGS_$(MODE))
        
# system-specific object files
PLATOBJS =                  \
	$(O)/i_main.o       \
	$(O)/i_sound.o      \
	$(O)/i_system.o     \
	$(O)/i_net.o

build : xsmmu ssmmu

# X version doesn't work on my pc currently
# please email me if you have any success
xsmmu: $(OBJS) $(O)/i_xwin.o $(O)/version.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(O)/i_xwin.o $(O)/version.o -o $@ -L/usr/X11/lib -lX11 -lXext

ssmmu: $(OBJS) $(O)/i_svga.o $(O)/version.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(O)/i_svga.o $(O)/version.o -o $@  -lvga

endif

nothing : 
	@echo SMMU makefile:
	@echo you need to specify a platform in the Makefile!

############################## OBJECT FILES #################################

# subdirectory for objects (depends on target, to allow you
# to build debug and release versions simultaneously)

# object files
OBJS=   \
	$(PLATOBJS)	    \
	$(O)/p_info.o	    \
	$(O)/c_cmd.o	    \
	$(O)/c_io.o	    \
	$(O)/c_runcmd.o	    \
	$(O)/c_net.o	    \
	$(O)/doomdef.o      \
        $(O)/doomstat.o     \
        $(O)/dstrings.o     \
        $(O)/tables.o       \
        $(O)/f_finale.o     \
        $(O)/f_wipe.o       \
        $(O)/d_main.o       \
        $(O)/d_net.o        \
        $(O)/d_items.o      \
        $(O)/g_game.o       \
	$(O)/g_cmd.o        \
        $(O)/mn_menus.o     \
	$(O)/mn_files.o     \
	$(O)/mn_misc.o      \
	$(O)/mn_engin.o	    \
        $(O)/m_misc.o       \
        $(O)/m_argv.o       \
        $(O)/m_bbox.o       \
        $(O)/m_cheat.o      \
        $(O)/m_random.o     \
        $(O)/am_map.o       \
	$(O)/am_color.o     \
        $(O)/p_ceilng.o     \
	$(O)/p_chase.o	    \
	$(O)/p_cmd.o	    \
        $(O)/p_doors.o      \
        $(O)/p_enemy.o      \
        $(O)/p_floor.o      \
	$(O)/p_hubs.o       \
        $(O)/p_inter.o      \
        $(O)/p_lights.o     \
        $(O)/p_map.o        \
        $(O)/p_maputl.o     \
        $(O)/p_plats.o      \
        $(O)/p_pspr.o       \
        $(O)/p_setup.o      \
        $(O)/p_sight.o      \
	$(O)/p_skin.o	    \
        $(O)/p_spec.o       \
        $(O)/p_switch.o     \
        $(O)/p_mobj.o       \
        $(O)/p_telept.o     \
        $(O)/p_tick.o       \
        $(O)/p_saveg.o      \
        $(O)/p_user.o       \
        $(O)/r_bsp.o        \
        $(O)/r_data.o       \
        $(O)/r_draw.o       \
        $(O)/r_main.o       \
        $(O)/r_plane.o      \
        $(O)/r_segs.o       \
	$(O)/r_ripple.o     \
        $(O)/r_sky.o        \
        $(O)/r_things.o     \
        $(O)/w_wad.o        \
        $(O)/wi_stuff.o     \
        $(O)/v_video.o      \
        $(O)/st_lib.o       \
        $(O)/st_stuff.o     \
        $(O)/hu_stuff.o     \
	$(O)/hu_over.o      \
	$(O)/hu_frags.o	    \
        $(O)/s_sound.o      \
        $(O)/z_zone.o       \
        $(O)/info.o         \
        $(O)/sounds.o       \
        $(O)/p_genlin.o     \
        $(O)/d_deh.o	    \
	$(O)/v_misc.o	    \
	$(O)/t_script.o     \
	$(O)/t_parse.o      \
	$(O)/t_prepro.o     \
	$(O)/t_vari.o	    \
	$(O)/t_func.o       \
	$(O)/t_oper.o	    \
        $(O)/t_spec.o

debug:
	$(MAKE) MODE=DEBUG

$(O)/%.o:   %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(O)/%.o:   $(PLATFORM)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(O)/%.o:   $(PLATFORM)/%.s
	$(CC) $(CFLAGS) -c $< -o $@

# Very important that all sources #include this one
$(OBJS): z_zone.h

# If you change the makefile, everything should rebuild
# $(OBJS): Makefile

# individual file depedencies follow

# dependencies currently broken :(
ifdef DEPENDENCIES

# rebuild version.c if anything changes
$(O)/version.o: version.c $(OBJS)

$(O)/doomdef.o: doomdef.c doomdef.h z_zone.h m_swap.h version.h

$(O)/doomstat.o: doomstat.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h \
 m_fixed.h i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h

$(O)/dstrings.o: dstrings.c dstrings.h d_englsh.h

$(O)/i_system.o: $(PLATFORM)/i_system.c i_system.h d_ticcmd.h doomtype.h \
 i_sound.h \
 sounds.h doomstat.h doomdata.h d_net.h d_player.h d_items.h doomdef.h \
 z_zone.h m_swap.h version.h p_pspr.h m_fixed.h tables.h info.h \
 d_think.h p_mobj.h m_misc.h g_game.h d_event.h w_wad.h v_video.h

$(O)/i_sound.o: $(PLATFORM)/i_sound.c z_zone.h doomstat.h doomdata.h \
 doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h m_swap.h version.h p_pspr.h m_fixed.h \
 i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h djgpp/mmus2mid.h \
 i_sound.h sounds.h w_wad.h g_game.h d_event.h d_main.h s_sound.h

$(O)/i_video.o: $(PLATFORM)/i_video.c z_zone.h doomstat.h doomdata.h \
 doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h m_swap.h version.h p_pspr.h m_fixed.h \
 i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h v_video.h \
 r_data.h r_defs.h r_state.h d_main.h d_event.h st_stuff.h m_argv.h w_wad.h \
 sounds.h s_sound.h r_draw.h am_map.h mn_engin.h wi_stuff.h

$(O)/i_net.o: $(PLATFORM)/i_net.c z_zone.h doomstat.h doomdata.h doomtype.h \
 d_net.h \
 d_player.h d_items.h doomdef.h m_swap.h version.h p_pspr.h m_fixed.h \
 i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h d_event.h \
 m_argv.h i_net.h

$(O)/tables.o: tables.c tables.h m_fixed.h i_system.h d_ticcmd.h doomtype.h

$(O)/f_finale.o: f_finale.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h \
 m_fixed.h i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h \
 d_event.h v_video.h r_data.h r_defs.h r_state.h w_wad.h s_sound.h \
 sounds.h dstrings.h d_englsh.h d_deh.h hu_stuff.h mn_engin.h

$(O)/f_wipe.o: f_wipe.c doomdef.h z_zone.h m_swap.h version.h i_video.h \
 doomtype.h v_video.h r_data.h r_defs.h m_fixed.h i_system.h \
 d_ticcmd.h d_think.h p_mobj.h tables.h doomdata.h info.h r_state.h \
 d_player.h d_items.h p_pspr.h m_random.h f_wipe.h

$(O)/d_main.o: d_main.c doomdef.h z_zone.h m_swap.h version.h doomstat.h \
 doomdata.h doomtype.h d_net.h d_player.h d_items.h p_pspr.h m_fixed.h \
 i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h dstrings.h \
 d_englsh.h sounds.h w_wad.h s_sound.h v_video.h r_data.h r_defs.h \
 r_state.h f_finale.h d_event.h f_wipe.h m_argv.h m_misc.h mn_engin.h \
 i_sound.h i_video.h g_game.h hu_stuff.h wi_stuff.h st_stuff.h \
 am_map.h p_setup.h r_draw.h r_main.h d_main.h d_deh.h

$(O)/d_net.o: d_net.c doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h m_fixed.h \
 i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h mn_engin.h \
 d_event.h i_video.h i_net.h g_game.h

$(O)/d_items.o: d_items.c info.h d_think.h d_items.h doomdef.h z_zone.h \
 m_swap.h version.h

$(O)/g_game.o: g_game.c doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h m_fixed.h \
 i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h f_finale.h \
 d_event.h m_argv.h m_misc.h mn_engin.h m_random.h p_setup.h p_saveg.h \
 p_tick.h d_main.h wi_stuff.h hu_stuff.h st_stuff.h am_map.h w_wad.h \
 r_main.h r_data.h r_defs.h r_state.h r_draw.h p_map.h s_sound.h \
 dstrings.h d_englsh.h sounds.h r_sky.h d_deh.h p_inter.h g_game.h

$(O)/mn_menus.o: mn_menus.c doomdef.h z_zone.h m_swap.h version.h doomstat.h \
 doomdata.h doomtype.h d_net.h d_player.h d_items.h p_pspr.h m_fixed.h \
 i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h dstrings.h \
 d_englsh.h d_main.h d_event.h i_video.h v_video.h r_data.h r_defs.h \
 r_state.h w_wad.h r_main.h hu_stuff.h g_game.h s_sound.h sounds.h \
 mn_engin.h d_deh.h m_misc.h

$(O)/m_misc.o: m_misc.c doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h m_fixed.h \
 i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h m_argv.h \
 g_game.h d_event.h mn_engin.h am_map.h w_wad.h i_sound.h sounds.h \
 i_video.h v_video.h r_data.h r_defs.h r_state.h hu_stuff.h st_stuff.h \
 dstrings.h d_englsh.h m_misc.h s_sound.h d_main.h

$(O)/m_argv.o: m_argv.c

$(O)/m_bbox.o: m_bbox.c m_bbox.h z_zone.h m_fixed.h i_system.h d_ticcmd.h \
 doomtype.h

$(O)/m_cheat.o: m_cheat.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h \
 m_fixed.h i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h \
 g_game.h d_event.h r_data.h r_defs.h r_state.h p_inter.h m_cheat.h \
 m_argv.h s_sound.h sounds.h dstrings.h d_englsh.h d_deh.h

$(O)/m_random.o: m_random.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h \
 m_fixed.h i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h \
 m_random.h

$(O)/am_map.o: am_map.c doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h m_fixed.h \
 i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h st_stuff.h \
 d_event.h r_main.h r_data.h r_defs.h r_state.h p_setup.h p_maputl.h \
 w_wad.h v_video.h p_spec.h am_map.h dstrings.h d_englsh.h d_deh.h

$(O)/p_ceilng.o: p_ceilng.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h \
 m_fixed.h i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h \
 r_main.h r_data.h r_defs.h r_state.h p_spec.h p_tick.h s_sound.h \
 sounds.h

$(O)/p_doors.o: p_doors.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h \
 m_fixed.h i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h \
 r_main.h r_data.h r_defs.h r_state.h p_spec.h p_tick.h s_sound.h \
 sounds.h dstrings.h d_englsh.h d_deh.h

$(O)/p_enemy.o: p_enemy.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h \
 m_fixed.h i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h \
 m_random.h r_main.h r_data.h r_defs.h r_state.h p_maputl.h p_map.h \
 p_setup.h p_spec.h s_sound.h sounds.h p_inter.h g_game.h d_event.h \
 p_enemy.h p_tick.h m_bbox.h

$(O)/p_floor.o: p_floor.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h \
 m_fixed.h i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h \
 r_main.h r_data.h r_defs.h r_state.h p_map.h p_spec.h p_tick.h \
 s_sound.h sounds.h

$(O)/p_inter.o: p_inter.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h \
 m_fixed.h i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h \
 dstrings.h d_englsh.h m_random.h am_map.h d_event.h r_main.h r_data.h \
 r_defs.h r_state.h s_sound.h sounds.h d_deh.h p_inter.h p_tick.h

$(O)/p_lights.o: p_lights.c doomdef.h z_zone.h m_swap.h version.h \
 m_random.h doomtype.h r_main.h d_player.h d_items.h p_pspr.h d_net.h \
 m_fixed.h i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h \
 doomdata.h r_data.h r_defs.h r_state.h p_spec.h p_tick.h doomstat.h

$(O)/p_map.o: p_map.c doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h m_fixed.h \
 i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h r_main.h \
 r_data.h r_defs.h r_state.h p_maputl.h p_map.h p_setup.h p_spec.h \
 s_sound.h sounds.h p_inter.h m_random.h m_bbox.h

$(O)/p_maputl.o: p_maputl.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h \
 m_fixed.h i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h \
 m_bbox.h r_main.h r_data.h r_defs.h r_state.h p_maputl.h p_map.h \
 p_setup.h

$(O)/p_plats.o: p_plats.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h \
 m_fixed.h i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h \
 m_random.h r_main.h r_data.h r_defs.h r_state.h p_spec.h p_tick.h \
 s_sound.h sounds.h

$(O)/p_pspr.o: p_pspr.c doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h m_fixed.h \
 i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h r_main.h \
 r_data.h r_defs.h r_state.h p_map.h p_inter.h p_enemy.h m_random.h \
 s_sound.h sounds.h d_event.h p_tick.h

$(O)/p_setup.o: p_setup.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h \
 m_fixed.h i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h \
 m_bbox.h m_argv.h g_game.h d_event.h w_wad.h r_main.h r_data.h \
 r_defs.h r_state.h r_things.h p_maputl.h p_map.h p_setup.h p_spec.h \
 p_tick.h p_enemy.h s_sound.h

$(O)/p_sight.o: p_sight.c r_main.h d_player.h d_items.h doomdef.h z_zone.h \
 m_swap.h version.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 doomtype.h tables.h info.h d_think.h p_mobj.h doomdata.h r_data.h \
 r_defs.h r_state.h p_maputl.h p_setup.h m_bbox.h

$(O)/p_spec.o: p_spec.c doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h m_fixed.h \
 i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h p_spec.h \
 r_defs.h p_tick.h p_setup.h m_random.h d_englsh.h m_argv.h w_wad.h \
 r_main.h r_data.h r_state.h p_maputl.h p_map.h g_game.h d_event.h \
 p_inter.h s_sound.h sounds.h m_bbox.h d_deh.h r_plane.h

$(O)/p_switch.o: p_switch.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h \
 m_fixed.h i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h \
 w_wad.h r_main.h r_data.h r_defs.h r_state.h p_spec.h g_game.h \
 d_event.h s_sound.h sounds.h

$(O)/p_mobj.o: p_mobj.c doomdef.h z_zone.h m_swap.h version.h doomstat.h \
 doomdata.h doomtype.h d_net.h d_player.h d_items.h p_pspr.h m_fixed.h \
 i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h m_random.h \
 r_main.h r_data.h r_defs.h r_state.h p_maputl.h p_map.h p_tick.h \
 sounds.h st_stuff.h d_event.h hu_stuff.h s_sound.h g_game.h p_inter.h

$(O)/p_telept.o: p_telept.c doomdef.h z_zone.h m_swap.h version.h p_spec.h \
 r_defs.h m_fixed.h i_system.h d_ticcmd.h doomtype.h d_think.h p_user.h \
 p_mobj.h tables.h doomdata.h info.h d_player.h d_items.h p_pspr.h \
 p_maputl.h p_map.h r_main.h r_data.h r_state.h p_tick.h s_sound.h \
 sounds.h doomstat.h d_net.h

$(O)/p_tick.o: p_tick.c z_zone.h doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h m_swap.h version.h p_pspr.h m_fixed.h \
 i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h p_user.h \
 p_spec.h r_defs.h p_tick.h

$(O)/p_saveg.o: p_saveg.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h \
 m_fixed.h i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h \
 r_main.h r_data.h r_defs.h r_state.h p_maputl.h p_spec.h p_tick.h \
 p_saveg.h m_random.h am_map.h d_event.h p_enemy.h

$(O)/p_user.o: p_user.c doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h m_fixed.h \
 i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h d_event.h \
 r_main.h r_data.h r_defs.h r_state.h p_map.h p_spec.h p_user.h

$(O)/r_bsp.o: r_bsp.c doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h m_fixed.h \
 i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h m_bbox.h \
 r_main.h r_data.h r_defs.h r_state.h r_segs.h r_plane.h r_things.h

$(O)/r_data.o: r_data.c doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h m_fixed.h \
 i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h w_wad.h \
 r_main.h r_data.h r_defs.h r_state.h r_sky.h

$(O)/r_draw.o: r_draw.c doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h m_fixed.h \
 i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h w_wad.h \
 r_main.h r_data.h r_defs.h r_state.h v_video.h mn_engin.h

$(O)/r_main.o: r_main.c doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h m_fixed.h \
 i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h r_data.h \
 r_defs.h r_state.h r_main.h r_bsp.h r_segs.h r_plane.h r_things.h \
 r_draw.h m_bbox.h r_sky.h v_video.h

$(O)/r_plane.o: r_plane.c z_zone.h i_system.h d_ticcmd.h doomtype.h w_wad.h \
 doomdef.h m_swap.h version.h doomstat.h doomdata.h d_net.h d_player.h \
 d_items.h p_pspr.h m_fixed.h tables.h info.h d_think.h p_mobj.h \
 r_plane.h r_data.h r_defs.h r_state.h r_main.h r_bsp.h r_segs.h \
 r_things.h r_draw.h r_sky.h

$(O)/r_segs.o: r_segs.c doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h m_fixed.h \
 i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h r_main.h \
 r_data.h r_defs.h r_state.h r_bsp.h r_plane.h r_things.h r_draw.h \
 w_wad.h

$(O)/r_sky.o: r_sky.c r_sky.h m_fixed.h i_system.h d_ticcmd.h doomtype.h

$(O)/r_things.o: r_things.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h \
 m_fixed.h i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h \
 w_wad.h r_main.h r_data.h r_defs.h r_state.h r_bsp.h r_segs.h \
 r_draw.h r_things.h

$(O)/w_wad.o: w_wad.c doomstat.h doomdata.h doomtype.h d_net.h d_player.h \
 d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h m_fixed.h \
 i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h w_wad.h

$(O)/wi_stuff.o: wi_stuff.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h \
 m_fixed.h i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h \
 m_random.h w_wad.h g_game.h d_event.h r_main.h r_data.h r_defs.h \
 r_state.h v_video.h wi_stuff.h s_sound.h sounds.h

$(O)/v_video.o: v_video.c doomdef.h z_zone.h m_swap.h version.h r_main.h \
 d_player.h d_items.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 doomtype.h tables.h info.h d_think.h p_mobj.h doomdata.h r_data.h \
 r_defs.h r_state.h m_bbox.h w_wad.h v_video.h i_video.h

$(O)/st_lib.o: st_lib.c doomdef.h z_zone.h m_swap.h version.h v_video.h \
 doomtype.h r_data.h r_defs.h m_fixed.h i_system.h d_ticcmd.h \
 d_think.h p_mobj.h tables.h doomdata.h info.h r_state.h d_player.h \
 d_items.h p_pspr.h w_wad.h st_stuff.h d_event.h st_lib.h r_main.h \
 r_bsp.h r_segs.h r_plane.h r_things.h r_draw.h

$(O)/st_stuff.o: st_stuff.c doomdef.h z_zone.h m_swap.h version.h \
 doomstat.h doomdata.h doomtype.h d_net.h d_player.h d_items.h \
 p_pspr.h m_fixed.h i_system.h d_ticcmd.h tables.h info.h d_think.h \
 p_mobj.h m_random.h i_video.h w_wad.h st_stuff.h d_event.h st_lib.h \
 r_defs.h v_video.h r_data.h r_state.h r_main.h am_map.h m_cheat.h \
 s_sound.h sounds.h dstrings.h d_englsh.h

$(O)/hu_stuff.o: hu_stuff.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h \
 m_fixed.h i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h \
 hu_stuff.h d_event.h  r_defs.h v_video.h r_data.h r_state.h \
 st_stuff.h w_wad.h s_sound.h dstrings.h d_englsh.h sounds.h d_deh.h \
 r_draw.h

$(O)/s_sound.o: s_sound.c doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h z_zone.h m_swap.h version.h p_pspr.h \
 m_fixed.h i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h \
 s_sound.h i_sound.h sounds.h r_main.h r_data.h r_defs.h r_state.h \
 m_random.h w_wad.h

$(O)/z_zone.o: z_zone.c z_zone.h doomstat.h doomdata.h doomtype.h d_net.h \
 d_player.h d_items.h doomdef.h m_swap.h version.h p_pspr.h m_fixed.h \
 i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h

$(O)/info.o: info.c doomdef.h z_zone.h m_swap.h version.h sounds.h \
 m_fixed.h i_system.h d_ticcmd.h doomtype.h p_mobj.h tables.h \
 d_think.h doomdata.h info.h w_wad.h

$(O)/sounds.o: sounds.c doomtype.h sounds.h

$(O)/mmus2mid.o: $(PLATFORM)/mmus2mid.c djgpp/mmus2mid.h z_zone.h

$(O)/i_main.o: $(PLATFORM)/i_main.c doomdef.h z_zone.h m_swap.h version.h m_argv.h \
 d_main.h d_event.h doomtype.h i_system.h d_ticcmd.h

$(O)/p_genlin.o: p_genlin.c r_main.h d_player.h d_items.h doomdef.h \
 z_zone.h m_swap.h version.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h \
 doomtype.h tables.h info.h d_think.h p_mobj.h doomdata.h r_data.h \
 r_defs.h r_state.h p_spec.h p_tick.h m_random.h s_sound.h sounds.h \
 doomstat.h

$(O)/d_deh.o: d_deh.c doomdef.h z_zone.h m_swap.h version.h doomstat.h \
 doomdata.h doomtype.h d_net.h d_player.h d_items.h p_pspr.h m_fixed.h \
 i_system.h d_ticcmd.h tables.h info.h d_think.h p_mobj.h sounds.h \
 m_cheat.h p_inter.h g_game.h d_event.h dstrings.h d_englsh.h w_wad.h

$(O)/version.o: version.c version.h z_zone.h

# Allegro patches required to function satisfactorily

$(O)/emu8kmid.o: $(PLATFORM)/emu8kmid.c djgpp/emu8k.h djgpp/internal.h \
  djgpp/interndj.h djgpp/allegro.h

$(O)/keyboard.o: $(PLATFORM)/keyboard.c djgpp/internal.h  \
  djgpp/interndj.h djgpp/allegro.h
	$(CC) $(CFLAGS_COMMON) -O $(CFLAGS_NEWFEATURES) -c $< -o $@

endif

# bin2c utility

bin2c: $(O)/bin2c.exe
	$(CP) $(O)\bin2c.exe .

$(O)/bin2c.exe: $(O)/bin2c.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(O)/bin2c.o -o $@ $(LIBS)

$(O)/bin2c.o: bin2c.c

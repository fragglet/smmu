#=========================================================================
#
# SMMU Makefile
#
#=========================================================================

MODE = RELEASE
DEPENDENCIES = 1

# 15/10/99 sf: added multi-platform support

# select platform here:
DJGPPDOS=1
#CYGWIN32=1
#LINUX=1

#-------------------------------------------------------------------------
#
# DJGPP (DOS)
#
#-------------------------------------------------------------------------

ifdef DJGPPDOS

PLATFORM = djgpp
        
# compiler
CC=gcc
        
# the command you use to delete files
RM=del
        
# the command you use to copy files
CP=copy /y
        
# the exe file name -sf
EXE=smmu.exe
        
# options common to all builds
CFLAGS_COMMON=-Wall -g

# new features; comment out what you don't want at the moment
# remove -DTCPIP if you want a version without tcp/ip support
CFLAGS_NEWFEATURES=-DDOGS -DTCPIP -DUSEASM
        
# debug options
CFLAGS_DEBUG=-g -O2 -DRANGECHECK -DINSTRUMENTED
LDFLAGS_DEBUG=
        
# optimized (release) options
CFLAGS_RELEASE=-O3 -ffast-math -fomit-frame-pointer -m486 -mreg-alloc=adcbSDB
LDFLAGS_RELEASE=
# -s
       
# libraries to link in
# remove -lsocket if you want a version w/out tcp/ip support
LIBS=-lalleg -lm -lemu -lsocket
        
# this selects flags based on debug and release tagets
CFLAGS=$(CFLAGS_COMMON)  $(CFLAGS_$(MODE)) $(CFLAGS_NEWFEATURES)
LDFLAGS=$(LDFLAGS_COMMON) $(LDFLAGS_$(MODE))
        
# system-specific object files



PLATOBJS =             \
	i_main.o       \
	i_system.o     \
	i_sound.o      \
	v_alleg.o      \
	v_vga.o        \
	v_text.o       \
	net_ser.o      \
	net_ext.o      \
	ser_port.o     \
        keyboard.o     \
	mmus2mid.o     \
	pproblit.o     \
	drawspan.o     \
	emu8kmid.o     \
	drawcol.o

build : $(EXE)

$(EXE): $(OBJS) version.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) version.o -o $@ $(LIBS)

release: sources binaries

sources:
# build source zip
	make clean
	$(RM) smmu-src.zip
	pkzip -a -ex -rp smmu-src

binaries:
# build binaries zip
	del smmu.exe
	make build
	strip smmu.exe
	djp smmu.exe
	if exist smmu.zip del smmu.zip
	pkzip -a -ex smmu smmu.exe smmu.wad               \
			smmu.txt smmuedit.txt                       \
			doomlic.txt copying copying.dj              \
			sock.vxd winsock2.bat
	pkzip -a -ex -rp smmu examples/*.wad

run: obj/smmu.exe
	obj/smmu.exe -iwad ..

clean:
	del *.c~
	del *.h~
	del tranmap.dat
	del smmu.cfg
	if exist $(O_RELEASE)\*.exe del $(O_RELEASE)\*.exe
	if exist $(O_DEBUG)\*.exe del $(O_DEBUG)\*.exe
	if exist $(O_RELEASE)\*.o del $(O_RELEASE)\*.o
	if exist $(O_DEBUG)\*.o del $(O_DEBUG)\*.o

endif

#------------------------------------------------------------------------
#
# Cygwin-32
#
# Works but: no sound, no Directx (!), no fullscreen, no networking
# (net ports are always blocking, even when told to be non-blocking?)
#
#------------------------------------------------------------------------

ifdef CYGWIN32

PLATFORM = cygwin
        
# compiler
CC=gcc
        
# the command you use to delete files
RM=del
        
# the command you use to copy files
CP=copy /y
        
# the exe file name -sf
EXE=smmu.exe
        
# options common to all builds
CFLAGS_COMMON=-Wall -g -DCYGWIN -DTCPIP
LDFLAGS_COMMON=

# new features; comment out what you don't want at the moment
# remove -DTCPIP if you want a version without tcp/ip support
CFLAGS_NEWFEATURES=-DDOGS
        
# debug options
CFLAGS_DEBUG=-g -O2 -DRANGECHECK -DINSTRUMENTED
LDFLAGS_DEBUG=
        
# optimized (release) options
CFLAGS_RELEASE=-O3 -ffast-math -fomit-frame-pointer -m486 -mreg-alloc=adcbSDB
LDFLAGS_RELEASE=
# -s
       
# libraries to link in
LIBS=-lcygwin -luser32 -lgdi32 -lcomdlg32 -lkernel32 -lwsock32 -lwinmm
        
# this selects flags based on debug and release tagets
CFLAGS=$(CFLAGS_COMMON)  $(CFLAGS_$(MODE)) $(CFLAGS_NEWFEATURES)
LDFLAGS=$(LDFLAGS_COMMON) $(LDFLAGS_$(MODE))
        
# system-specific object files

PLATOBJS =             \
	i_main.o       \
	i_system.o     \
	i_sound.o      \
	v_win32.o

build : $(EXE)

$(EXE): $(OBJS) version.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) version.o -o $@ $(LIBS)

endif

#------------------------------------------------------------------------
#
# Linux
#
# nb. no sound support yet :(
#
#------------------------------------------------------------------------

ifdef LINUX

PLATFORM = linux
# compiler
CC = gcc
        
# the command you use to delete files
RM = rm
# the command you use to copy files
CP = cp
                
# options common to all builds
CFLAGS_COMMON = -Wall -g -DLINUX

# new features; comment out what you don't want at the moment
CFLAGS_NEWFEATURES = -DDOGS -DTCPIP -DUSEASM

# debug options
CFLAGS_DEBUG = -g -O2 -DRANGECHECK -DINSTRUMENTED
LDFLAGS_DEBUG =
        
# optimized (release) options
CFLAGS_RELEASE = -O2
LDFLAGS_RELEASE =
# -s
        
# this selects flags based on debug and release targets
CFLAGS=$(CFLAGS_COMMON)  $(CFLAGS_$(MODE)) $(CFLAGS_NEWFEATURES)
LDFLAGS=$(LDFLAGS_COMMON) $(LDFLAGS_$(MODE))
        
# system-specific object files
PLATOBJS =             \
	i_main.o       \
	i_sound.o      \
	v_xwin.o       \
	v_svga.o       \
	i_system.o     \
	drawcol.o      \
	drawspan.o

#----------------- graphics drivers --------------------

unknown:
	@echo specify graphics type:
	@echo 'make x' for X Window version
	@echo 'make s' for svgalib version
	@echo 'make xs' for version which supports both

xs:
	make smmu \
LDFLAGS="$(LDFLAGS) -L/usr/X11/lib -lX11 -lXext -lvga" \
CFLAGS="$(CFLAGS) -DSVGA -DXWIN"

s:
	make smmu \
LDFLAGS="$(LDFLAGS) -lvga" \
CFLAGS="$(CFLAGS) -DSVGA"

x:
	make smmu \
LDFLAGS="$(LDFLAGS) -L/usr/X11/lib -lX11 -lXext" \
CFLAGS="$(CFLAGS) -DXWIN"

#----------------------- main build ------------------------------

smmu : $(OBJS) version.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) version.o -o $@

# dedicated server

dedicated : 
	make -f dedserv.mak

endif

nothing : 
	@echo SMMU makefile:
	@echo you need to specify a platform in the Makefile!

#---------------------------------------------------------------------------
#
# Object Files
#
#---------------------------------------------------------------------------

# subdirectory for objects (depends on target, to allow you
# to build debug and release versions simultaneously)

# object files
OBJS=   \
	$(PLATOBJS)    \
	p_info.o       \
	c_cmd.o	       \
	c_io.o	       \
	c_runcmd.o     \
	c_net.o	       \
	doomdef.o      \
	doomstat.o     \
	dstrings.o     \
	tables.o       \
	f_finale.o     \
	f_wipe.o       \
	d_main.o       \
	d_items.o      \
	g_game.o       \
	g_bind.o       \
	g_cmd.o        \
	mn_menus.o     \
	mn_files.o     \
	mn_misc.o      \
	mn_net.o       \
	mn_engin.o     \
	m_misc.o       \
	m_argv.o       \
	m_bbox.o       \
	m_cheat.o      \
	m_random.o     \
	am_map.o       \
	am_color.o     \
	p_ceilng.o     \
	p_chase.o      \
	p_cmd.o	       \
	p_doors.o      \
	p_enemy.o      \
	p_floor.o      \
	p_hubs.o       \
	p_inter.o      \
	p_lights.o     \
	p_map.o        \
	p_maputl.o     \
	p_plats.o      \
	p_pspr.o       \
	p_setup.o      \
	p_sight.o      \
	p_skin.o       \
	p_spec.o       \
	p_switch.o     \
	p_mobj.o       \
	p_telept.o     \
	p_tick.o       \
	p_saveg.o      \
	p_user.o       \
	r_bsp.o        \
	r_data.o       \
	r_draw.o       \
	r_main.o       \
	r_plane.o      \
	r_segs.o       \
	r_ripple.o     \
	r_sky.o        \
	r_things.o     \
	w_wad.o        \
	wi_stuff.o     \
	v_video.o      \
	v_mode.o       \
	st_lib.o       \
	st_stuff.o     \
	hu_stuff.o     \
	hu_over.o      \
	hu_frags.o     \
	s_sound.o      \
	z_zone.o       \
	info.o         \
	sounds.o       \
	p_genlin.o     \
	d_deh.o	       \
	v_misc.o       \
	t_script.o     \
	t_parse.o      \
	t_prepro.o     \
	t_vari.o       \
	t_func.o       \
	t_oper.o       \
	t_spec.o       \
	cl_clien.o     \
	cl_demo.o      \
	cl_find.o      \
	sv_serv.o      \
	net_gen.o      \
	net_udp.o      \
	net_loop.o

debug:
	$(MAKE) MODE=DEBUG

%.o:   %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o:   $(PLATFORM)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o:   $(PLATFORM)/%.s
	$(CC) $(CFLAGS) -c $< -o $@

# Very important that all sources #include this one
$(OBJS): z_zone.h

# If you change the makefile, everything should rebuild
# $(OBJS): Makefile

# individual file depedencies follow

# dependencies currently broken :(
ifdef DEPENDENCIES

# rebuild version.c if anything changes

version.o: version.c $(OBJS)

# fixed dependencies  : 12/8/2000

am_color.o:	am_color.c doomdef.h z_zone.h m_swap.h version.h c_runcmd.h
am_map.o:	am_map.c c_io.h d_event.h doomtype.h v_video.h doomdef.h\
	z_zone.h m_swap.h version.h r_data.h r_defs.h m_fixed.h i_system.h\
	d_ticcmd.h d_think.h p_mobj.h tables.h doomdata.h info.h p_skin.h\
	sounds.h st_stuff.h d_player.h d_items.h p_pspr.h r_state.h p_chase.h\
	v_misc.h c_runcmd.h doomstat.h cl_clien.h sv_serv.h d_main.h r_main.h\
	p_setup.h p_maputl.h r_draw.h w_wad.h p_spec.h am_map.h dstrings.h\
	d_englsh.h d_deh.h
c_cmd.o:	c_cmd.c c_io.h d_event.h doomtype.h v_video.h doomdef.h\
	z_zone.h m_swap.h version.h r_data.h r_defs.h m_fixed.h i_system.h\
	d_ticcmd.h d_think.h p_mobj.h tables.h doomdata.h info.h p_skin.h\
	sounds.h st_stuff.h d_player.h d_items.h p_pspr.h r_state.h p_chase.h\
	v_misc.h c_runcmd.h c_net.h m_random.h
c_io.o:	c_io.c c_io.h d_event.h doomtype.h v_video.h doomdef.h z_zone.h\
	m_swap.h version.h r_data.h r_defs.h m_fixed.h i_system.h d_ticcmd.h\
	d_think.h p_mobj.h tables.h doomdata.h info.h p_skin.h sounds.h\
	st_stuff.h d_player.h d_items.h p_pspr.h r_state.h p_chase.h v_misc.h\
	c_runcmd.h c_net.h d_main.h g_game.h g_bind.h hu_stuff.h hu_over.h\
	doomstat.h cl_clien.h sv_serv.h v_mode.h w_wad.h s_sound.h
c_net.o:	c_net.c c_io.h d_event.h doomtype.h v_video.h doomdef.h\
	z_zone.h m_swap.h version.h r_data.h r_defs.h m_fixed.h i_system.h\
	d_ticcmd.h d_think.h p_mobj.h tables.h doomdata.h info.h p_skin.h\
	sounds.h st_stuff.h d_player.h d_items.h p_pspr.h r_state.h p_chase.h\
	v_misc.h c_runcmd.h c_net.h d_main.h g_game.h doomstat.h cl_clien.h\
	sv_serv.h dstrings.h d_englsh.h
c_runcmd.o:	c_runcmd.c c_io.h d_event.h doomtype.h v_video.h doomdef.h\
	z_zone.h m_swap.h version.h r_data.h r_defs.h m_fixed.h i_system.h\
	d_ticcmd.h d_think.h p_mobj.h tables.h doomdata.h info.h p_skin.h\
	sounds.h st_stuff.h d_player.h d_items.h p_pspr.h r_state.h p_chase.h\
	v_misc.h c_runcmd.h c_net.h doomstat.h cl_clien.h sv_serv.h mn_engin.h\
	t_script.h t_parse.h t_vari.h t_prepro.h g_game.h
cl_clien.o:	cl_clien.c z_zone.h am_map.h d_event.h doomtype.h m_fixed.h\
	i_system.h d_ticcmd.h c_io.h v_video.h doomdef.h m_swap.h version.h\
	r_data.h r_defs.h d_think.h p_mobj.h tables.h doomdata.h info.h\
	p_skin.h sounds.h st_stuff.h d_player.h d_items.h p_pspr.h r_state.h\
	p_chase.h v_misc.h c_net.h c_runcmd.h doomstat.h cl_clien.h sv_serv.h\
	d_deh.h d_main.h f_wipe.h g_game.h hu_stuff.h m_random.h mn_engin.h\
	p_user.h r_draw.h s_sound.h net_gen.h net_modl.h
cl_demo.o:	cl_demo.c c_io.h d_event.h doomtype.h v_video.h doomdef.h\
	z_zone.h m_swap.h version.h r_data.h r_defs.h m_fixed.h i_system.h\
	d_ticcmd.h d_think.h p_mobj.h tables.h doomdata.h info.h p_skin.h\
	sounds.h st_stuff.h d_player.h d_items.h p_pspr.h r_state.h p_chase.h\
	v_misc.h c_runcmd.h doomstat.h cl_clien.h sv_serv.h d_main.h g_game.h\
	m_argv.h m_misc.h m_random.h w_wad.h
cl_find.o:	cl_find.c c_io.h d_event.h doomtype.h v_video.h doomdef.h\
	z_zone.h m_swap.h version.h r_data.h r_defs.h m_fixed.h i_system.h\
	d_ticcmd.h d_think.h p_mobj.h tables.h doomdata.h info.h p_skin.h\
	sounds.h st_stuff.h d_player.h d_items.h p_pspr.h r_state.h p_chase.h\
	v_misc.h c_runcmd.h cl_clien.h sv_serv.h mn_engin.h net_gen.h\
	net_modl.h
d_deh.o:	d_deh.c doomdef.h z_zone.h m_swap.h version.h doomstat.h\
	cl_clien.h d_event.h doomtype.h sv_serv.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h m_cheat.h\
	p_inter.h g_game.h w_wad.h dstrings.h d_englsh.h
d_items.o:	d_items.c info.h d_think.h d_items.h doomdef.h z_zone.h\
	m_swap.h version.h
d_main.o:	d_main.c doomdef.h z_zone.h m_swap.h version.h doomstat.h\
	cl_clien.h d_event.h doomtype.h sv_serv.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h am_map.h\
	c_io.h v_video.h r_data.h r_state.h p_chase.h v_misc.h c_net.h\
	c_runcmd.h dstrings.h d_englsh.h d_deh.h d_main.h f_finale.h f_wipe.h\
	g_bind.h g_game.h hu_stuff.h i_sound.h m_argv.h m_misc.h mn_engin.h\
	p_setup.h r_draw.h r_main.h s_sound.h t_script.h t_parse.h t_vari.h\
	t_prepro.h v_mode.h w_wad.h wi_stuff.h
dedserv.o:	dedserv.c doomdef.h z_zone.h m_swap.h version.h net_modl.h\
	sv_serv.h d_ticcmd.h doomtype.h
doomdef.o:	doomdef.c doomdef.h z_zone.h m_swap.h version.h
doomstat.o:	doomstat.c doomstat.h cl_clien.h d_event.h doomtype.h\
	sv_serv.h doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h
dstrings.o:	dstrings.c dstrings.h d_englsh.h
f_finale.o:	f_finale.c doomstat.h cl_clien.h d_event.h doomtype.h\
	sv_serv.h doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h v_video.h\
	r_data.h r_state.h p_chase.h v_misc.h w_wad.h s_sound.h dstrings.h\
	d_englsh.h mn_engin.h c_runcmd.h d_deh.h p_info.h c_io.h
f_wipe.o:	f_wipe.c c_io.h d_event.h doomtype.h v_video.h doomdef.h\
	z_zone.h m_swap.h version.h r_data.h r_defs.h m_fixed.h i_system.h\
	d_ticcmd.h d_think.h p_mobj.h tables.h doomdata.h info.h p_skin.h\
	sounds.h st_stuff.h d_player.h d_items.h p_pspr.h r_state.h p_chase.h\
	v_misc.h d_main.h m_random.h f_wipe.h
g_bind.o:	g_bind.c doomdef.h z_zone.h m_swap.h version.h doomstat.h\
	cl_clien.h d_event.h doomtype.h sv_serv.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h c_io.h\
	v_video.h r_data.h r_state.h p_chase.h v_misc.h c_runcmd.h d_deh.h\
	g_game.h mn_engin.h mn_misc.h m_misc.h w_wad.h
g_cmd.o:	g_cmd.c doomdef.h z_zone.h m_swap.h version.h doomstat.h\
	cl_clien.h d_event.h doomtype.h sv_serv.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h d_main.h\
	p_chase.h c_io.h v_video.h r_data.h r_state.h v_misc.h c_runcmd.h\
	c_net.h f_wipe.h g_game.h hu_stuff.h mn_engin.h m_random.h p_inter.h\
	p_setup.h w_wad.h
g_game.o:	g_game.c c_io.h d_event.h doomtype.h v_video.h doomdef.h\
	z_zone.h m_swap.h version.h r_data.h r_defs.h m_fixed.h i_system.h\
	d_ticcmd.h d_think.h p_mobj.h tables.h doomdata.h info.h p_skin.h\
	sounds.h st_stuff.h d_player.h d_items.h p_pspr.h r_state.h p_chase.h\
	v_misc.h c_net.h c_runcmd.h p_info.h doomstat.h cl_clien.h sv_serv.h\
	f_finale.h f_wipe.h m_argv.h m_misc.h mn_engin.h mn_menus.h m_random.h\
	p_setup.h p_saveg.h p_tick.h d_main.h wi_stuff.h hu_stuff.h am_map.h\
	w_wad.h r_main.h r_draw.h p_map.h s_sound.h dstrings.h d_englsh.h\
	r_sky.h d_deh.h p_inter.h g_game.h g_bind.h p_hubs.h
hu_frags.o:	hu_frags.c hu_frags.h c_io.h d_event.h doomtype.h v_video.h\
	doomdef.h z_zone.h m_swap.h version.h r_data.h r_defs.h m_fixed.h\
	i_system.h d_ticcmd.h d_think.h p_mobj.h tables.h doomdata.h info.h\
	p_skin.h sounds.h st_stuff.h d_player.h d_items.h p_pspr.h r_state.h\
	p_chase.h v_misc.h c_runcmd.h doomstat.h cl_clien.h sv_serv.h g_game.h\
	r_draw.h w_wad.h
hu_over.o:	hu_over.c doomdef.h z_zone.h m_swap.h version.h doomstat.h\
	cl_clien.h d_event.h doomtype.h sv_serv.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h c_runcmd.h\
	d_deh.h g_game.h hu_frags.h hu_over.h hu_stuff.h v_video.h r_data.h\
	r_state.h p_chase.h v_misc.h p_info.h c_io.h p_map.h p_setup.h\
	r_draw.h s_sound.h w_wad.h
hu_stuff.o:	hu_stuff.c doomdef.h z_zone.h m_swap.h version.h doomstat.h\
	cl_clien.h d_event.h doomtype.h sv_serv.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h c_net.h\
	c_runcmd.h d_deh.h g_game.h hu_frags.h hu_stuff.h v_video.h r_data.h\
	r_state.h p_chase.h v_misc.h hu_over.h p_info.h c_io.h p_map.h\
	p_setup.h p_spec.h r_draw.h s_sound.h w_wad.h
info.o:	info.c doomdef.h z_zone.h m_swap.h version.h sounds.h m_fixed.h\
	i_system.h d_ticcmd.h doomtype.h p_mobj.h tables.h d_think.h\
	doomdata.h info.h p_skin.h st_stuff.h d_event.h r_defs.h d_player.h\
	d_items.h p_pspr.h w_wad.h
m_argv.o:	m_argv.c
m_bbox.o:	m_bbox.c m_bbox.h z_zone.h doomtype.h m_fixed.h i_system.h\
	d_ticcmd.h
m_cheat.o:	m_cheat.c doomstat.h cl_clien.h d_event.h doomtype.h\
	sv_serv.h doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h c_runcmd.h\
	c_net.h g_game.h r_data.h r_state.h p_chase.h p_inter.h m_cheat.h\
	m_argv.h s_sound.h dstrings.h d_englsh.h d_deh.h
m_misc.o:	m_misc.c doomstat.h cl_clien.h d_event.h doomtype.h sv_serv.h\
	doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h d_player.h\
	d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h d_think.h\
	p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h m_argv.h g_game.h\
	mn_engin.h c_runcmd.h am_map.h w_wad.h i_sound.h v_mode.h v_video.h\
	r_data.h r_state.h p_chase.h v_misc.h hu_stuff.h dstrings.h d_englsh.h\
	m_misc.h s_sound.h d_main.h r_draw.h c_io.h c_net.h
m_random.o:	m_random.c doomstat.h cl_clien.h d_event.h doomtype.h\
	sv_serv.h doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h m_random.h
mn_engin.o:	mn_engin.c doomdef.h z_zone.h m_swap.h version.h doomstat.h\
	cl_clien.h d_event.h doomtype.h sv_serv.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h c_io.h\
	v_video.h r_data.h r_state.h p_chase.h v_misc.h c_runcmd.h d_main.h\
	g_bind.h g_game.h hu_over.h mn_engin.h mn_menus.h mn_misc.h r_draw.h\
	s_sound.h w_wad.h
mn_files.o:	mn_files.c c_io.h d_event.h doomtype.h v_video.h doomdef.h\
	z_zone.h m_swap.h version.h r_data.h r_defs.h m_fixed.h i_system.h\
	d_ticcmd.h d_think.h p_mobj.h tables.h doomdata.h info.h p_skin.h\
	sounds.h st_stuff.h d_player.h d_items.h p_pspr.h r_state.h p_chase.h\
	v_misc.h mn_engin.h c_runcmd.h
mn_menus.o:	mn_menus.c doomdef.h z_zone.h m_swap.h version.h doomstat.h\
	cl_clien.h d_event.h doomtype.h sv_serv.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h c_io.h\
	v_video.h r_data.h r_state.h p_chase.h v_misc.h c_runcmd.h d_deh.h\
	d_main.h dstrings.h d_englsh.h g_game.h hu_over.h m_random.h\
	mn_engin.h mn_misc.h r_draw.h s_sound.h w_wad.h v_mode.h
mn_misc.o:	mn_misc.c doomstat.h cl_clien.h d_event.h doomtype.h\
	sv_serv.h doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h d_main.h\
	p_chase.h p_info.h c_io.h v_video.h r_data.h r_state.h v_misc.h\
	s_sound.h w_wad.h mn_engin.h c_runcmd.h mn_misc.h
mn_net.o:	mn_net.c doomdef.h z_zone.h m_swap.h version.h doomstat.h\
	cl_clien.h d_event.h doomtype.h sv_serv.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h d_main.h\
	p_chase.h m_argv.h mn_engin.h c_runcmd.h net_modl.h r_draw.h v_video.h\
	r_data.h r_state.h v_misc.h w_wad.h
net_gen.o:	net_gen.c c_io.h d_event.h doomtype.h v_video.h doomdef.h\
	z_zone.h m_swap.h version.h r_data.h r_defs.h m_fixed.h i_system.h\
	d_ticcmd.h d_think.h p_mobj.h tables.h doomdata.h info.h p_skin.h\
	sounds.h st_stuff.h d_player.h d_items.h p_pspr.h r_state.h p_chase.h\
	v_misc.h c_runcmd.h m_random.h net_gen.h sv_serv.h net_modl.h
net_loop.o:	net_loop.c doomdef.h z_zone.h m_swap.h version.h sv_serv.h\
	d_ticcmd.h doomtype.h
net_udp.o:	net_udp.c
p_ceilng.o:	p_ceilng.c doomstat.h cl_clien.h d_event.h doomtype.h\
	sv_serv.h doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h r_main.h\
	r_data.h r_state.h p_chase.h p_spec.h p_tick.h s_sound.h
p_chase.o:	p_chase.c c_io.h d_event.h doomtype.h v_video.h doomdef.h\
	z_zone.h m_swap.h version.h r_data.h r_defs.h m_fixed.h i_system.h\
	d_ticcmd.h d_think.h p_mobj.h tables.h doomdata.h info.h p_skin.h\
	sounds.h st_stuff.h d_player.h d_items.h p_pspr.h r_state.h p_chase.h\
	v_misc.h c_runcmd.h cl_clien.h sv_serv.h doomstat.h d_main.h p_map.h\
	p_maputl.h r_main.h g_game.h
p_cmd.o:	p_cmd.c c_io.h d_event.h doomtype.h v_video.h doomdef.h\
	z_zone.h m_swap.h version.h r_data.h r_defs.h m_fixed.h i_system.h\
	d_ticcmd.h d_think.h p_mobj.h tables.h doomdata.h info.h p_skin.h\
	sounds.h st_stuff.h d_player.h d_items.h p_pspr.h r_state.h p_chase.h\
	v_misc.h c_net.h c_runcmd.h doomstat.h cl_clien.h sv_serv.h d_main.h\
	f_wipe.h g_game.h m_random.h p_info.h p_inter.h p_spec.h r_draw.h
p_doors.o:	p_doors.c doomstat.h cl_clien.h d_event.h doomtype.h\
	sv_serv.h doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h g_game.h\
	p_spec.h p_tick.h s_sound.h r_main.h r_data.h r_state.h p_chase.h\
	dstrings.h d_englsh.h d_deh.h hu_stuff.h v_video.h v_misc.h
p_enemy.o:	p_enemy.c doomstat.h cl_clien.h d_event.h doomtype.h\
	sv_serv.h doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h m_random.h\
	r_main.h r_data.h r_state.h p_chase.h p_maputl.h p_map.h p_setup.h\
	p_spec.h s_sound.h p_inter.h g_game.h p_enemy.h p_user.h p_tick.h\
	m_bbox.h t_script.h t_parse.h t_vari.h t_prepro.h
p_floor.o:	p_floor.c doomstat.h cl_clien.h d_event.h doomtype.h\
	sv_serv.h doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h c_io.h\
	v_video.h r_data.h r_state.h p_chase.h v_misc.h r_main.h p_map.h\
	p_spec.h p_tick.h s_sound.h
p_genlin.o:	p_genlin.c doomstat.h cl_clien.h d_event.h doomtype.h\
	sv_serv.h doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h r_main.h\
	r_data.h r_state.h p_chase.h p_spec.h p_tick.h m_random.h s_sound.h
p_hubs.o:	p_hubs.c doomstat.h cl_clien.h d_event.h doomtype.h sv_serv.h\
	doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h d_player.h\
	d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h d_think.h\
	p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h c_io.h v_video.h\
	r_data.h r_state.h p_chase.h v_misc.h g_game.h p_maputl.h p_setup.h\
	p_spec.h r_main.h t_vari.h t_parse.h t_prepro.h
p_info.o:	p_info.c doomstat.h cl_clien.h d_event.h doomtype.h sv_serv.h\
	doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h d_player.h\
	d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h d_think.h\
	p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h c_io.h v_video.h\
	r_data.h r_state.h p_chase.h v_misc.h c_runcmd.h d_deh.h p_setup.h\
	p_info.h t_script.h t_parse.h t_vari.h t_prepro.h w_wad.h
p_inter.o:	p_inter.c c_io.h d_event.h doomtype.h v_video.h doomdef.h\
	z_zone.h m_swap.h version.h r_data.h r_defs.h m_fixed.h i_system.h\
	d_ticcmd.h d_think.h p_mobj.h tables.h doomdata.h info.h p_skin.h\
	sounds.h st_stuff.h d_player.h d_items.h p_pspr.h r_state.h p_chase.h\
	v_misc.h doomstat.h cl_clien.h sv_serv.h dstrings.h d_englsh.h\
	m_random.h g_game.h hu_stuff.h hu_frags.h am_map.h p_user.h r_main.h\
	r_segs.h s_sound.h p_tick.h d_deh.h p_inter.h
p_lights.o:	p_lights.c doomstat.h cl_clien.h d_event.h doomtype.h\
	sv_serv.h doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h m_random.h\
	r_main.h r_data.h r_state.h p_chase.h p_spec.h p_tick.h
p_map.o:	p_map.c doomstat.h cl_clien.h d_event.h doomtype.h sv_serv.h\
	doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h d_player.h\
	d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h d_think.h\
	p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h r_main.h r_data.h\
	r_state.h p_chase.h p_maputl.h p_map.h p_setup.h p_spec.h p_user.h\
	s_sound.h p_inter.h m_random.h r_segs.h m_bbox.h
p_maputl.o:	p_maputl.c doomstat.h cl_clien.h d_event.h doomtype.h\
	sv_serv.h doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h m_bbox.h\
	r_main.h r_data.h r_state.h p_chase.h p_maputl.h p_map.h p_setup.h
p_mobj.o:	p_mobj.c doomdef.h z_zone.h m_swap.h version.h doomstat.h\
	cl_clien.h d_event.h doomtype.h sv_serv.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h m_random.h\
	r_main.h r_data.h r_state.h p_chase.h p_maputl.h p_map.h p_tick.h\
	hu_stuff.h v_video.h v_misc.h s_sound.h g_game.h p_inter.h p_user.h\
	wi_stuff.h
p_plats.o:	p_plats.c doomstat.h cl_clien.h d_event.h doomtype.h\
	sv_serv.h doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h m_random.h\
	r_main.h r_data.h r_state.h p_chase.h p_spec.h p_tick.h s_sound.h
p_pspr.o:	p_pspr.c doomstat.h cl_clien.h d_event.h doomtype.h sv_serv.h\
	doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h d_player.h\
	d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h d_think.h\
	p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h g_game.h m_random.h\
	p_enemy.h p_inter.h p_map.h p_maputl.h p_tick.h p_user.h r_main.h\
	r_data.h r_state.h p_chase.h r_segs.h r_things.h s_sound.h
p_saveg.o:	p_saveg.c doomstat.h cl_clien.h d_event.h doomtype.h\
	sv_serv.h doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h r_main.h\
	r_data.h r_state.h p_chase.h p_maputl.h p_spec.h p_tick.h p_saveg.h\
	m_random.h am_map.h p_enemy.h p_hubs.h t_vari.h t_parse.h t_prepro.h\
	t_script.h
p_setup.o:	p_setup.c c_io.h d_event.h doomtype.h v_video.h doomdef.h\
	z_zone.h m_swap.h version.h r_data.h r_defs.h m_fixed.h i_system.h\
	d_ticcmd.h d_think.h p_mobj.h tables.h doomdata.h info.h p_skin.h\
	sounds.h st_stuff.h d_player.h d_items.h p_pspr.h r_state.h p_chase.h\
	v_misc.h c_runcmd.h d_main.h hu_stuff.h wi_stuff.h doomstat.h\
	cl_clien.h sv_serv.h hu_frags.h m_bbox.h m_argv.h g_game.h w_wad.h\
	p_hubs.h r_main.h r_things.h r_sky.h p_maputl.h p_map.h p_setup.h\
	p_spec.h p_tick.h p_enemy.h p_info.h s_sound.h t_script.h t_parse.h\
	t_vari.h t_prepro.h
p_sight.o:	p_sight.c r_main.h d_player.h d_items.h doomdef.h z_zone.h\
	m_swap.h version.h p_pspr.h m_fixed.h i_system.h d_ticcmd.h doomtype.h\
	tables.h info.h d_think.h p_mobj.h doomdata.h p_skin.h sounds.h\
	st_stuff.h d_event.h r_defs.h r_data.h r_state.h p_chase.h p_maputl.h\
	p_setup.h m_bbox.h
p_skin.o:	p_skin.c c_runcmd.h c_io.h d_event.h doomtype.h v_video.h\
	doomdef.h z_zone.h m_swap.h version.h r_data.h r_defs.h m_fixed.h\
	i_system.h d_ticcmd.h d_think.h p_mobj.h tables.h doomdata.h info.h\
	p_skin.h sounds.h st_stuff.h d_player.h d_items.h p_pspr.h r_state.h\
	p_chase.h v_misc.h c_net.h doomstat.h cl_clien.h sv_serv.h d_main.h\
	p_info.h r_things.h s_sound.h w_wad.h
p_spec.o:	p_spec.c doomstat.h cl_clien.h d_event.h doomtype.h sv_serv.h\
	doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h d_player.h\
	d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h d_think.h\
	p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h p_spec.h p_tick.h\
	p_setup.h m_random.h d_englsh.h m_argv.h w_wad.h r_main.h r_data.h\
	r_state.h p_chase.h p_maputl.h p_map.h g_game.h p_inter.h s_sound.h\
	m_bbox.h d_deh.h r_plane.h p_info.h c_io.h v_video.h v_misc.h\
	c_runcmd.h hu_stuff.h t_script.h t_parse.h t_vari.h t_prepro.h\
	r_ripple.h
p_switch.o:	p_switch.c doomstat.h cl_clien.h d_event.h doomtype.h\
	sv_serv.h doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h g_game.h\
	p_spec.h r_main.h r_data.h r_state.h p_chase.h s_sound.h t_script.h\
	t_parse.h t_vari.h t_prepro.h w_wad.h
p_telept.o:	p_telept.c doomstat.h cl_clien.h d_event.h doomtype.h\
	sv_serv.h doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h p_chase.h\
	p_maputl.h p_map.h p_spec.h r_main.h r_data.h r_state.h p_tick.h\
	s_sound.h p_user.h
p_tick.o:	p_tick.c doomstat.h cl_clien.h d_event.h doomtype.h sv_serv.h\
	doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h d_player.h\
	d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h d_think.h\
	p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h d_main.h p_chase.h\
	p_user.h p_spec.h p_tick.h t_script.h t_parse.h t_vari.h t_prepro.h
p_user.o:	p_user.c doomstat.h cl_clien.h d_event.h doomtype.h sv_serv.h\
	doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h d_player.h\
	d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h d_think.h\
	p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h c_net.h c_runcmd.h\
	g_game.h hu_stuff.h v_video.h r_data.h r_state.h p_chase.h v_misc.h\
	r_main.h p_map.h p_maputl.h p_spec.h p_user.h
r_bsp.o:	r_bsp.c doomstat.h cl_clien.h d_event.h doomtype.h sv_serv.h\
	doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h d_player.h\
	d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h d_think.h\
	p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h m_bbox.h r_main.h\
	r_data.h r_state.h p_chase.h r_segs.h r_plane.h r_things.h
r_data.o:	r_data.c doomstat.h cl_clien.h d_event.h doomtype.h sv_serv.h\
	doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h d_player.h\
	d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h d_think.h\
	p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h c_io.h v_video.h\
	r_data.h r_state.h p_chase.h v_misc.h d_main.h w_wad.h p_setup.h\
	r_main.h r_sky.h
r_draw.o:	r_draw.c doomstat.h cl_clien.h d_event.h doomtype.h sv_serv.h\
	doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h d_player.h\
	d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h d_think.h\
	p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h w_wad.h r_draw.h\
	r_main.h r_data.h r_state.h p_chase.h v_video.h v_misc.h mn_engin.h\
	c_runcmd.h
r_main.o:	r_main.c doomstat.h cl_clien.h d_event.h doomtype.h sv_serv.h\
	doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h d_player.h\
	d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h d_think.h\
	p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h c_runcmd.h g_game.h\
	hu_over.h mn_engin.h r_main.h r_data.h r_state.h p_chase.h r_things.h\
	r_plane.h r_ripple.h r_bsp.h r_draw.h m_bbox.h r_sky.h s_sound.h\
	v_mode.h v_video.h v_misc.h w_wad.h
r_plane.o:	r_plane.c z_zone.h doomstat.h cl_clien.h d_event.h doomtype.h\
	sv_serv.h doomdef.h m_swap.h version.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h c_io.h\
	v_video.h r_data.h r_state.h p_chase.h v_misc.h w_wad.h r_main.h\
	r_draw.h r_things.h r_sky.h r_ripple.h r_plane.h
r_ripple.o:	r_ripple.c doomdef.h z_zone.h m_swap.h version.h doomstat.h\
	cl_clien.h d_event.h doomtype.h sv_serv.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h r_data.h\
	r_state.h p_chase.h w_wad.h v_video.h v_misc.h
r_segs.o:	r_segs.c doomstat.h cl_clien.h d_event.h doomtype.h sv_serv.h\
	doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h d_player.h\
	d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h d_think.h\
	p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h r_main.h r_data.h\
	r_state.h p_chase.h r_bsp.h r_plane.h r_things.h r_draw.h w_wad.h
r_sky.o:	r_sky.c doomstat.h cl_clien.h d_event.h doomtype.h sv_serv.h\
	doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h d_player.h\
	d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h d_think.h\
	p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h r_sky.h r_data.h\
	r_state.h p_chase.h p_info.h c_io.h v_video.h v_misc.h
r_things.o:	r_things.c c_io.h d_event.h doomtype.h v_video.h doomdef.h\
	z_zone.h m_swap.h version.h r_data.h r_defs.h m_fixed.h i_system.h\
	d_ticcmd.h d_think.h p_mobj.h tables.h doomdata.h info.h p_skin.h\
	sounds.h st_stuff.h d_player.h d_items.h p_pspr.h r_state.h p_chase.h\
	v_misc.h doomstat.h cl_clien.h sv_serv.h w_wad.h g_game.h d_main.h\
	r_main.h r_bsp.h r_segs.h r_draw.h r_things.h
s_sound.o:	s_sound.c doomstat.h cl_clien.h d_event.h doomtype.h\
	sv_serv.h doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h d_main.h\
	p_chase.h s_sound.h i_sound.h r_main.h r_data.h r_state.h m_random.h\
	w_wad.h c_io.h v_video.h v_misc.h c_runcmd.h p_info.h
sounds.o:	sounds.c doomtype.h sounds.h p_skin.h info.h d_think.h\
	st_stuff.h d_event.h r_defs.h doomdef.h z_zone.h m_swap.h version.h\
	m_fixed.h i_system.h d_ticcmd.h p_mobj.h tables.h doomdata.h\
	d_player.h d_items.h p_pspr.h
st_lib.o:	st_lib.c doomdef.h z_zone.h m_swap.h version.h doomstat.h\
	cl_clien.h d_event.h doomtype.h sv_serv.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h v_video.h\
	r_data.h r_state.h p_chase.h v_misc.h w_wad.h st_lib.h r_main.h
st_stuff.o:	st_stuff.c doomdef.h z_zone.h m_swap.h version.h doomstat.h\
	cl_clien.h d_event.h doomtype.h sv_serv.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h c_runcmd.h\
	d_main.h p_chase.h m_random.h w_wad.h st_lib.h v_video.h r_data.h\
	r_state.h v_misc.h r_main.h am_map.h m_cheat.h s_sound.h dstrings.h\
	d_englsh.h v_mode.h
sv_serv.o:	sv_serv.c c_io.h d_event.h doomtype.h v_video.h doomdef.h\
	z_zone.h m_swap.h version.h r_data.h r_defs.h m_fixed.h i_system.h\
	d_ticcmd.h d_think.h p_mobj.h tables.h doomdata.h info.h p_skin.h\
	sounds.h st_stuff.h d_player.h d_items.h p_pspr.h r_state.h p_chase.h\
	v_misc.h c_runcmd.h c_net.h cl_clien.h sv_serv.h mn_engin.h net_gen.h\
	net_modl.h
t_func.o:	t_func.c c_io.h d_event.h doomtype.h v_video.h doomdef.h\
	z_zone.h m_swap.h version.h r_data.h r_defs.h m_fixed.h i_system.h\
	d_ticcmd.h d_think.h p_mobj.h tables.h doomdata.h info.h p_skin.h\
	sounds.h st_stuff.h d_player.h d_items.h p_pspr.h r_state.h p_chase.h\
	v_misc.h c_runcmd.h doomstat.h cl_clien.h sv_serv.h d_main.h g_game.h\
	hu_stuff.h m_random.h p_tick.h p_spec.h p_hubs.h p_inter.h r_main.h\
	r_segs.h s_sound.h w_wad.h t_parse.h t_vari.h t_prepro.h t_spec.h\
	t_script.h t_oper.h t_func.h
t_oper.o:	t_oper.c c_io.h d_event.h doomtype.h v_video.h doomdef.h\
	z_zone.h m_swap.h version.h r_data.h r_defs.h m_fixed.h i_system.h\
	d_ticcmd.h d_think.h p_mobj.h tables.h doomdata.h info.h p_skin.h\
	sounds.h st_stuff.h d_player.h d_items.h p_pspr.h r_state.h p_chase.h\
	v_misc.h doomstat.h cl_clien.h sv_serv.h t_parse.h t_vari.h t_prepro.h
t_parse.o:	t_parse.c c_io.h d_event.h doomtype.h v_video.h doomdef.h\
	z_zone.h m_swap.h version.h r_data.h r_defs.h m_fixed.h i_system.h\
	d_ticcmd.h d_think.h p_mobj.h tables.h doomdata.h info.h p_skin.h\
	sounds.h st_stuff.h d_player.h d_items.h p_pspr.h r_state.h p_chase.h\
	v_misc.h s_sound.h w_wad.h t_parse.h t_vari.h t_prepro.h t_spec.h\
	t_oper.h t_func.h
t_prepro.o:	t_prepro.c c_io.h d_event.h doomtype.h v_video.h doomdef.h\
	z_zone.h m_swap.h version.h r_data.h r_defs.h m_fixed.h i_system.h\
	d_ticcmd.h d_think.h p_mobj.h tables.h doomdata.h info.h p_skin.h\
	sounds.h st_stuff.h d_player.h d_items.h p_pspr.h r_state.h p_chase.h\
	v_misc.h doomstat.h cl_clien.h sv_serv.h w_wad.h t_parse.h t_vari.h\
	t_prepro.h t_spec.h t_oper.h t_func.h
t_script.o:	t_script.c doomstat.h cl_clien.h d_event.h doomtype.h\
	sv_serv.h doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h c_io.h\
	v_video.h r_data.h r_state.h p_chase.h v_misc.h c_net.h c_runcmd.h\
	p_info.h p_spec.h w_wad.h t_script.h t_parse.h t_vari.h t_prepro.h\
	t_func.h
t_spec.o:	t_spec.c c_io.h d_event.h doomtype.h v_video.h doomdef.h\
	z_zone.h m_swap.h version.h r_data.h r_defs.h m_fixed.h i_system.h\
	d_ticcmd.h d_think.h p_mobj.h tables.h doomdata.h info.h p_skin.h\
	sounds.h st_stuff.h d_player.h d_items.h p_pspr.h r_state.h p_chase.h\
	v_misc.h t_parse.h t_vari.h t_prepro.h t_spec.h
t_vari.o:	t_vari.c z_zone.h t_script.h p_mobj.h tables.h m_fixed.h\
	i_system.h d_ticcmd.h doomtype.h d_think.h doomdata.h info.h p_skin.h\
	sounds.h st_stuff.h d_event.h r_defs.h doomdef.h m_swap.h version.h\
	d_player.h d_items.h p_pspr.h t_parse.h t_vari.h t_prepro.h t_func.h
tables.o:	tables.c tables.h m_fixed.h i_system.h d_ticcmd.h doomtype.h
v_misc.o:	v_misc.c c_io.h d_event.h doomtype.h v_video.h doomdef.h\
	z_zone.h m_swap.h version.h r_data.h r_defs.h m_fixed.h i_system.h\
	d_ticcmd.h d_think.h p_mobj.h tables.h doomdata.h info.h p_skin.h\
	sounds.h st_stuff.h d_player.h d_items.h p_pspr.h r_state.h p_chase.h\
	v_misc.h c_runcmd.h doomstat.h cl_clien.h sv_serv.h v_mode.h w_wad.h
v_mode.o:	v_mode.c c_io.h d_event.h doomtype.h v_video.h doomdef.h\
	z_zone.h m_swap.h version.h r_data.h r_defs.h m_fixed.h i_system.h\
	d_ticcmd.h d_think.h p_mobj.h tables.h doomdata.h info.h p_skin.h\
	sounds.h st_stuff.h d_player.h d_items.h p_pspr.h r_state.h p_chase.h\
	v_misc.h c_runcmd.h am_map.h m_argv.h doomstat.h cl_clien.h sv_serv.h\
	r_main.h v_mode.h w_wad.h wi_stuff.h
v_video.o:	v_video.c c_io.h d_event.h doomtype.h v_video.h doomdef.h\
	z_zone.h m_swap.h version.h r_data.h r_defs.h m_fixed.h i_system.h\
	d_ticcmd.h d_think.h p_mobj.h tables.h doomdata.h info.h p_skin.h\
	sounds.h st_stuff.h d_player.h d_items.h p_pspr.h r_state.h p_chase.h\
	v_misc.h doomstat.h cl_clien.h sv_serv.h r_main.h m_bbox.h r_draw.h\
	w_wad.h v_mode.h
version.o:	version.c version.h
w_wad.o:	w_wad.c doomstat.h cl_clien.h d_event.h doomtype.h sv_serv.h\
	doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h d_player.h\
	d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h d_think.h\
	p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h c_io.h v_video.h\
	r_data.h r_state.h p_chase.h v_misc.h w_wad.h
wi_stuff.o:	wi_stuff.c doomstat.h cl_clien.h d_event.h doomtype.h\
	sv_serv.h doomdef.h z_zone.h m_swap.h version.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h m_random.h\
	w_wad.h g_game.h r_main.h r_data.h r_state.h p_chase.h p_info.h c_io.h\
	v_video.h v_misc.h wi_stuff.h s_sound.h hu_stuff.h am_map.h p_tick.h
z_zone.o:	z_zone.c z_zone.h doomstat.h cl_clien.h d_event.h doomtype.h\
	sv_serv.h doomdef.h m_swap.h version.h d_ticcmd.h doomdata.h\
	d_player.h d_items.h p_pspr.h m_fixed.h i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h sounds.h st_stuff.h r_defs.h
linux/i_main.o:	linux/i_main.c doomdef.h z_zone.h\
	m_swap.h version.h m_argv.h\
	d_main.h d_event.h doomtype.h\
	p_chase.h p_mobj.h tables.h\
	m_fixed.h i_system.h d_ticcmd.h\
	d_think.h doomdata.h info.h\
	p_skin.h sounds.h st_stuff.h\
	r_defs.h d_player.h d_items.h\
	p_pspr.h
linux/i_sound.o:	linux/i_sound.c c_runcmd.h doomstat.h\
	cl_clien.h d_event.h doomtype.h\
	sv_serv.h doomdef.h z_zone.h\
	m_swap.h version.h d_ticcmd.h\
	doomdata.h d_player.h d_items.h\
	p_pspr.h m_fixed.h i_system.h\
	tables.h info.h d_think.h p_mobj.h\
	p_skin.h sounds.h st_stuff.h\
	r_defs.h i_sound.h w_wad.h\
	g_game.h d_main.h p_chase.h\
	linux/i_esound.h
linux/i_system.o:	linux/i_system.c c_runcmd.h\
	i_system.h d_ticcmd.h doomtype.h\
	i_sound.h sounds.h doomstat.h\
	cl_clien.h d_event.h sv_serv.h\
	doomdef.h z_zone.h m_swap.h\
	version.h doomdata.h d_player.h\
	d_items.h p_pspr.h m_fixed.h\
	tables.h info.h d_think.h p_mobj.h\
	p_skin.h st_stuff.h r_defs.h\
	m_misc.h g_game.h w_wad.h\
	v_video.h r_data.h r_state.h\
	p_chase.h v_misc.h m_argv.h
linux/v_svga.o:	linux/v_svga.c
linux/v_xwin.o:	linux/v_xwin.c
cygwin/i_main.o:	cygwin/i_main.c doomdef.h z_zone.h\
	m_swap.h version.h m_argv.h\
	d_main.h d_event.h doomtype.h\
	p_chase.h p_mobj.h tables.h\
	m_fixed.h i_system.h d_ticcmd.h\
	d_think.h doomdata.h info.h\
	p_skin.h sounds.h st_stuff.h\
	r_defs.h d_player.h d_items.h\
	p_pspr.h
cygwin/i_sound.o:	cygwin/i_sound.c c_runcmd.h\
	doomstat.h cl_clien.h d_event.h\
	doomtype.h sv_serv.h doomdef.h\
	z_zone.h m_swap.h version.h\
	d_ticcmd.h doomdata.h d_player.h\
	d_items.h p_pspr.h m_fixed.h\
	i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h\
	sounds.h st_stuff.h r_defs.h\
	i_sound.h w_wad.h g_game.h\
	d_main.h p_chase.h
cygwin/i_system.o:	cygwin/i_system.c c_runcmd.h\
	i_system.h d_ticcmd.h doomtype.h\
	i_sound.h sounds.h doomstat.h\
	cl_clien.h d_event.h sv_serv.h\
	doomdef.h z_zone.h m_swap.h\
	version.h doomdata.h d_player.h\
	d_items.h p_pspr.h m_fixed.h\
	tables.h info.h d_think.h\
	p_mobj.h p_skin.h st_stuff.h\
	r_defs.h m_misc.h g_game.h\
	w_wad.h v_video.h r_data.h\
	r_state.h p_chase.h v_misc.h\
	m_argv.h
cygwin/v_win32.o:	cygwin/v_win32.c doomtype.h\
	doomdef.h z_zone.h m_swap.h\
	version.h m_argv.h d_event.h\
	d_main.h p_chase.h p_mobj.h\
	tables.h m_fixed.h i_system.h\
	d_ticcmd.h d_think.h doomdata.h\
	info.h p_skin.h sounds.h\
	st_stuff.h r_defs.h d_player.h\
	d_items.h p_pspr.h v_video.h\
	r_data.h r_state.h v_misc.h\
	v_mode.h doomstat.h cl_clien.h\
	sv_serv.h
djgpp/emu8kmid.o:	djgpp/emu8kmid.c djgpp/allegro.h djgpp/internal.h\
	djgpp/emu8k.h
djgpp/i_main.o:	djgpp/i_main.c doomdef.h z_zone.h\
	m_swap.h version.h m_argv.h\
	d_main.h d_event.h doomtype.h\
	p_chase.h p_mobj.h tables.h\
	m_fixed.h i_system.h d_ticcmd.h\
	d_think.h doomdata.h info.h\
	p_skin.h sounds.h st_stuff.h\
	r_defs.h d_player.h d_items.h\
	p_pspr.h
djgpp/i_sound.o:	djgpp/i_sound.c djgpp/mmus2mid.h c_io.h\
	d_event.h doomtype.h v_video.h\
	doomdef.h z_zone.h m_swap.h\
	version.h r_data.h r_defs.h\
	m_fixed.h i_system.h d_ticcmd.h\
	d_think.h p_mobj.h tables.h\
	doomdata.h info.h p_skin.h\
	sounds.h st_stuff.h d_player.h\
	d_items.h p_pspr.h r_state.h\
	p_chase.h v_misc.h c_runcmd.h\
	doomstat.h cl_clien.h sv_serv.h\
	i_sound.h w_wad.h g_game.h\
	d_main.h
djgpp/i_system.o:	djgpp/i_system.c djgpp/keyboard.h c_runcmd.h\
	i_system.h d_ticcmd.h doomtype.h\
	i_sound.h sounds.h doomstat.h\
	cl_clien.h d_event.h sv_serv.h\
	doomdef.h z_zone.h m_swap.h\
	version.h doomdata.h d_player.h\
	d_items.h p_pspr.h m_fixed.h\
	tables.h info.h d_think.h p_mobj.h\
	p_skin.h st_stuff.h r_defs.h\
	g_bind.h g_game.h m_argv.h\
	m_misc.h v_mode.h v_video.h\
	r_data.h r_state.h p_chase.h\
	v_misc.h w_wad.h
djgpp/keyboard.o:	djgpp/keyboard.c djgpp/internal.h djgpp/allegro.h
djgpp/mmus2mid.o:	djgpp/mmus2mid.c djgpp/mmus2mid.h z_zone.h
djgpp/net_ext.o:	djgpp/net_ext.c i_system.h d_ticcmd.h\
	doomtype.h m_argv.h m_random.h\
	sv_serv.h doomdef.h z_zone.h\
	m_swap.h version.h
djgpp/net_ser.o:	djgpp/net_ser.c djgpp/ser_port.h c_io.h\
	d_event.h doomtype.h v_video.h\
	doomdef.h z_zone.h m_swap.h\
	version.h r_data.h r_defs.h\
	m_fixed.h i_system.h d_ticcmd.h\
	d_think.h p_mobj.h tables.h\
	doomdata.h info.h p_skin.h\
	sounds.h st_stuff.h d_player.h\
	d_items.h p_pspr.h r_state.h\
	p_chase.h v_misc.h c_runcmd.h\
	d_main.h sv_serv.h
djgpp/ser_port.o:	djgpp/ser_port.c djgpp/ser_port.h d_main.h\
	d_event.h doomtype.h p_chase.h\
	p_mobj.h tables.h m_fixed.h\
	i_system.h d_ticcmd.h d_think.h\
	doomdata.h info.h p_skin.h\
	sounds.h st_stuff.h r_defs.h\
	doomdef.h z_zone.h m_swap.h\
	version.h d_player.h d_items.h\
	p_pspr.h c_io.h v_video.h r_data.h\
	r_state.h v_misc.h
djgpp/v_alleg.o:	djgpp/v_alleg.c z_zone.h djgpp/keyboard.h\
	doomdef.h m_swap.h version.h\
	doomstat.h cl_clien.h d_event.h\
	doomtype.h sv_serv.h d_ticcmd.h\
	doomdata.h d_player.h d_items.h\
	p_pspr.h m_fixed.h i_system.h\
	tables.h info.h d_think.h p_mobj.h\
	p_skin.h sounds.h st_stuff.h\
	r_defs.h c_io.h v_video.h r_data.h\
	r_state.h p_chase.h v_misc.h\
	c_runcmd.h d_main.h m_argv.h\
	v_mode.h w_wad.h
djgpp/v_vga.o:	djgpp/v_vga.c djgpp/keyboard.h doomdef.h\
	z_zone.h m_swap.h version.h\
	d_event.h doomtype.h v_mode.h\
	doomstat.h cl_clien.h sv_serv.h\
	d_ticcmd.h doomdata.h d_player.h\
	d_items.h p_pspr.h m_fixed.h\
	i_system.h tables.h info.h\
	d_think.h p_mobj.h p_skin.h\
	sounds.h st_stuff.h r_defs.h\
	v_video.h r_data.h r_state.h\
	p_chase.h v_misc.h


endif

# bin2c utility

bin2c: bin2c.exe
	$(CP) bin2c.exe .

bin2c.exe: bin2c.o
	$(CC) $(CFLAGS) $(LDFLAGS) bin2c.o -o $@ $(LIBS)

bin2c.o: bin2c.c

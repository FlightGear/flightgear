noinst_LIBRARIES = libCockpit.a

libCockpit_a_SOURCES = \
	cockpit.cxx cockpit.hxx \
	hud.cxx hud.hxx hud_opts.hxx \
	hud_card.cxx hud_dnst.cxx hud_gaug.cxx hud_inst.cxx \
	hud_labl.cxx hud_ladr.cxx \
	hud_lat.cxx hud_lon.cxx \
	hud_scal.cxx hud_tbi.cxx \
	kr_87.cxx kr_87.hxx \
	panel.cxx panel.hxx \
        panel_io.cxx panel_io.hxx \
	radiostack.cxx radiostack.hxx \
	steam.cxx steam.hxx

if OLD_AUTOMAKE
INCLUDES += -I$(top_srcdir) -I$(top_srcdir)/src
else
INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
endif

SUBDIRS = built_in

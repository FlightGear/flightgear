CPPFLAGS	=
CFLAGS		=
CC		= gcc
RM		= rm
PROGS		= fgd fgd_scan fgd_com fgd_mcp fgd_verbose
BINDIR		= /usr/local/bin

all: $(PROGS)

fgd:      fgd.c
	  $(CC) -o $@ $^

fgd_verbose: fgd_verbose.c
	  $(CC) -o $@ $^

fgd_scan: fgd_scan.c
	  $(CC) -o $@ $^

fgd_com:  fgd_com.c
	  $(CC) -o $@ $^

fgd_mcp:  fgd_mcp.c
	  $(CC) -o $@ $^

clean:
	-$(RM) -f *.o $(PROGS) *~ core

install: all
	install $(PROGS) $(BINDIR)
	
dist:	clean
	tar czf ../`basename \`pwd\``.tar.gz -C .. `basename \`pwd\``

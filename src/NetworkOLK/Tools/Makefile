CPPFLAGS	=
CFLAGS		=
CC		= gcc
RM		= rm
PROGS		= fgd fgd_scan fgd_com
BINDIR		= /usr/local/bin

all: $(PROGS)

fgd:      fgd.c
	  $(CC) -o $@ $^

fgd_scan: fgd_scan.c
	  $(CC) -o $@ $^

fgd_com:  fgd_com.c
	  $(CC) -o $@ $^

clean:
	-$(RM) -f *.o $(PROGS) *~ core

install: all devs
	install -d $(BINDIR)
	install $(PROGS) $(BINDIR)
	install -d $(MANDIR)/man4
	install -m 644 js.4 $(MANDIR)/man4
	
dist:	clean
	tar czf ../`basename \`pwd\``.tar.gz -C .. `basename \`pwd\``


#!/bin/sh

for n in `find . -name Makefile`; do \
   echo "Fixing file $n"; \
   mv -f $n $n.ar-new; \
   sed 's/$(AR) cru /$(AR) -o /g' $n.ar-new > $n; \
   rm -f $n.ar-new; \
done

#!/bin/sh

for n in `find . -name Makefile`; do \
   echo "Fixing file $n"; \
   \
   mv -f $n $n.ar-new; \
   sed 's/^AR = ar/AR = CC -ar/g' $n.ar-new > $n;
   \
   mv -f $n $n.ar-new; \
   sed 's/$(AR) cru /$(AR) -o /g' $n.ar-new > $n; \
done

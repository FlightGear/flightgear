#! /bin/bash

## 
## 

if test "x$1" = "x-h" ; then
  1>&2 echo "Usage: "
  1>&2 echo "	$0 [filename]"
  1>&2 echo "Read words from input, treating all whitespace like,"
  1>&2 echo "and write exactly N words per line on output."
  1>&2 echo "Options:  "
  1>&2 echo "	-n [N]	specify N (default: 1)"
  1>&2 echo "	filename = '-' or '' ==> read from standard input"
  exit 1
fi

: ${wordmax:=1}
files=""


while test -n "$*" ; do
  this=$1 ; shift
  case $this in 
    -n) wordmax=$1 ; shift
    ;;
    *) files="$files $this"
    ;;
  esac
done


awk '{
  for (ii = 1; ii <=NF; ii++) {
    printf ("%s", $ii);
    words++;
    if (words >= wordmax) {
      print "";
      words = 0;
    } else {
      printf (" ");
    }
  }
}'  wordmax=$wordmax $files

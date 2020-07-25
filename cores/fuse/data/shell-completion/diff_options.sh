#!/bin/sh

# diff_options.sh: diff shell-completion and man options with settings.dat

echo BASH
echo ====

# --help and --version options are not stored as settings
sed -e '1,/if \[\[ "$cur" == -\* \]\]; then/d' < bash/fuse | \
  grep -o -E '\-\-[a-Za-z0-9\-]+' | sort |
  grep -v -e "\-\-help" -e "\-\-version" > bash.txt

awk < ../../settings.dat \
  'BEGIN{ FS = "," } \
   { \
     if( $1 ~ "^#" ) next; \
     if( $2 ~ "null" ) next; \
     if( $5 != "" ) option = $5; else option = $1; \
     gsub( /^[ \t]+/, "", option ); \
     if( option != "" ) { \
       print option;
       if( $2 ~ "boolean" ) print "no-" option; \
     }
   }' | \
  sed -e 's/_/\-/g' -e 's/^/\-\-/' | sort > settings.txt

diff -U 0 bash.txt settings.txt

echo
echo MAN
echo ===

grep -o -E '\\\-\\\-[a-Za-z0-9\\\-]+' ../../man/fuse.1 | \
  sed -e 's/\\\-/\-/g' -e 's/\-\-no\-/\-\-/g' | sort | uniq | \
  grep -v -e "\-\-disable\-ui\-joystick" -e "\-\-foo" -e "\-\-help" \
          -e "\-\-version" > man.txt

grep -v -e "\-\-no\-" settings.txt | diff -U 0 man.txt -

#!/bin/sh
N=5
iterations=10000

make clean all
rm ./.times
for i in `seq 1 $N`;
do
  /usr/bin/time -f "%e real" -a -p -o ./.times ./gol $iterations inputs/1k.pbm outputs/1k.pbm > /dev/null
  diff ./outputs/1k.pbm ./outputs/1k_verify_out.pbm
done
echo "Average real :"
cat ./.times | grep real | sed 's/real //' | awk "{s+=\$1} END {print s/$N}"

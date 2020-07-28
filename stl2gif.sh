#!/bin/bash

#TODO check for -o opt in $@ if found take next and use that as file out name
mkdir /tmp/stl2gif
outfile=`./_stl2gif $@`
echo $outfile
for f in /tmp/stl2gif/*.bmp; 
do 
    convert $f $f.jpg;
    rm $f;
    #echo $f.jpg;
done;
convert -delay 10 -loop 0 /tmp/stl2gif/*.jpg $outfile
#cleanup
rm -R /tmp/stl2gif/


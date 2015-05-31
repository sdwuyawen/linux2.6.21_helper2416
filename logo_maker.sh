#!/bin/bash

if [ $# -lt 2 ]; then
echo usage: logo_maker [input file] [output file]
else
bmptoppm $1 > logo.ppm
ppmquant 224 logo.ppm >logo2.ppm
pnmnoraw logo2.ppm > $2
rm logo.ppm logo2.ppm
fi

#! /usr/bin/env python3

import sys

outfile = sys.argv[1]
infile = sys.argv[2]
start_byte = int(sys.argv[3], 16)
length = int(sys.argv[4], 16)

data = open(infile, "rb").read()

if len(data) < length:
    data += b'\0' * ( length - len(data) )

with open(outfile, "wb") as fp:
    fp.write(data[start_byte:start_byte+length:2])

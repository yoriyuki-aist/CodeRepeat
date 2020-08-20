#!/usr/bin/env python3
import argparse
import json
import sys
sys.stdin.reconfigure(encoding="ascii", errors="surrogateescape")

parser = argparse.ArgumentParser(description='Filter repeats')
parser.add_argument('-m', '--min', metavar='N', type=int, default=0,
                    help='minimal size of repeats')
parser.add_argument('-c', '--occ', type=int, default=0,
                    help='minimal number of occurrences')
parser.add_argument('-o', '--output', type=argparse.FileType('w', encoding="ascii", errors="surrogateescape"), default=sys.stdout,
                    help='output json file')
parser.add_argument('-i', '--input', type=argparse.FileType('r', encoding="ascii", errors="surrogateescape"), default=sys.stdin,
                    help='input json file')

args = parser.parse_args()

for line in args.input:
    ob = json.loads(line)
    if len(ob['text']) >= args.min and len(ob['locations']) >= args.occ:
        s = json.dumps(ob, separators=(',', ':'))
        args.output.write(s)
        args.output.write('\n')
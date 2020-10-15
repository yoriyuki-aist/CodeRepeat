#!/usr/bin/env python3
import argparse
import json
import io
import sys
input_stream = io.TextIOWrapper(sys.stdin.buffer, encoding='backslashreplace')
output_stream = io.TextIOWrapper(sys.stdout.buffer, encoding='backslashreplace')


parser = argparse.ArgumentParser(description='Filter repeats')
parser.add_argument('-l', '--length', dest='length', action='store_true',
                    help='top N longest repeats')
parser.add_argument('-c', '--occ', dest='occ', action='store_true',
                    help='top N frequent repeats')
parser.add_argument('-N', '--topN', type=int, metavar='N',
                    help='number of repeats to be find')
parser.add_argument('-o', '--output', type=argparse.FileType('w', encoding="ascii", errors='backslashreplace'), default=output_stream,
                    help='output json file')
parser.add_argument('-i', '--input', type=argparse.FileType('r', encoding="ascii", errors='backslashreplace'), default=input_stream,
                    help='input json file')

args = parser.parse_args()

if (args.length and args.occ) or ((not args.length) and (not args.occ)):
    sys.exit('Choose one of --len or --occ')  

ranking = []
current = 0

if args.length:
    key = 'text'
elif args.occ:
    key = 'locations'
else:
    sys.exit('impossible')

for line in args.input:
    ob = json.loads(line)
    v = len(ob[key])
    if v > current:
        ranking.append(ob)
        ranking = sorted(ranking, key=lambda ob: len(ob[key]), reverse=True)
        ranking = ranking[:args.topN]
        curent = len(ranking[-1][key])

for ob in ranking:
    s = json.dumps(ob, separators=(',', ':'))
    args.output.write(s)
    args.output.write('\n')
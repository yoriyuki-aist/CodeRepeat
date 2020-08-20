#!/usr/bin/env python3
import argparse
import json
import os
import sys
sys.stdin.reconfigure(encoding="ascii", errors="surrogateescape")

parser = argparse.ArgumentParser(description='Generate dataset for BigCloneBench')
parser.add_argument('-o', '--output', type=argparse.FileType('w', encoding="ascii", errors="surrogateescape"), default=sys.stdout,
                    help='output json file')
parser.add_argument('-i', '--input', type=argparse.FileType('r', encoding="ascii", errors="surrogateescape"), default=sys.stdin,
                    help='input json file')

args = parser.parse_args()

def output_repeat(repeat):
    dirname = os.path.dirname(repeat['path'])
    filename = os.path.basename(repeat['path'])
    start_line = repeat['start_line']
    end_line = repeat['end_line']
    args.output.write("{}, {}, {}, {}".format(dirname, filename, start_line, end_line))


for line in args.input:
    ob = json.loads(line)
    repeats = ob['locations']
    for i in range(len(repeats)):
        for j in range(i+1, len(repeats)):
            output_repeat(repeats[i])
            args.output.write(', ')
            output_repeat(repeats[j])
            args.output.write('\n')
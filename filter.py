#!/usr/bin/env python3
import ijson
import jsonstreams
import argparse

parser = argparse.ArgumentParser(description='Filter repeats')
parser.add_argument('-m', '--min', metavar='N', type=int,
                    help='minimal size of repeats')
parser.add_argument('-c', '--occ', type=int,
                    help='minimal number of occurrences')
parser.add_argument('-o', '--output', type=str,
                    help='output file')
parser.add_argument('json_file', type=str, help='input json file')

args = parser.parse_args()

with jsonstreams.Stream(jsonstreams.Type.object, filename=args.output) as s:
    s.write('ver', 0)
    ## Ugly
    with open(args.json_file, encoding='ascii', errors='ignore') as f:
        file_starts = ijson.items(f, 'file_starts')
        s.write('file_starts', next(file_starts))

    with s.subarray('repeats') as a:
        with open(args.json_file, encoding='ascii', errors='ignore') as f:
            repeats = ijson.items(f, 'repeats.item')
            for repeat in repeats:
                if len(repeat['text']) > args.min and len(repeat['positions']) > args.occ:
                    a.write(repeat)


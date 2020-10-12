#!/usr/bin/env python3
import argparse
import json
import os
import sys
from collections import namedtuple
from tqdm import tqdm

sys.stdin.reconfigure(encoding="ascii", errors="surrogateescape")


def output_repeat(repeat):
    dirname = os.path.dirname(repeat.path)
    filename = os.path.basename(repeat.path)
    start_line = repeat.start
    end_line = repeat.end
    if args.bigclonebench:
        dirname = os.path.basename(dirname)
        args.output.write("{},{},{},{}".format(dirname, filename, start_line, end_line))
    else:
        args.output.write("{},{},{},{}".format(dirname, filename, start_line, end_line))


def output_pair(pair):
    output_repeat(pair[0])
    args.output.write(',')
    output_repeat(pair[1])
    args.output.write('\n')


#Both sides are inclusive
Region = namedtuple('Pair', ['path', 'start', 'end'])
def convert_region(pair):
    return Region(pair['path'], pair['start_line'], pair['end_line'])


def normalize_pair(pair):
    p = (convert_region(pair[0]), convert_region(pair[1]))
    return sorted(p)


def get_key(pair):
    return (pair[0].path, pair[1].path)


def is_contacted(p1, p2):
    if p1[0].path != p2[0].path or p1[1].path != p2[1].path:
        return False

    if p1[0].start < p2[0].start and p1[0].end >= p2[0].end - args.gap - 1:
        return p1[1].start < p2[1].start and p1[1].end >= p2[1].end - args.gap - 1
    elif p1[0].start >= p2[0].start and p1[0].start <= p2[0].end:
        return p1[1].start >= p2[1].start and p1[1].start <= p2[1].end
    elif p1[0].start <= p2[0].end + args.gap + 1:
        return p1[1].start <= p2[1].end + args.gap + 1
    else:
        return False


def contacted_pairs(pair, pair_list):
    return list(filter(lambda p: is_contacted(p, pair), pair_list))


def merge_contacted_pair_list(pair_list):
    keys = list(map(get_key, pair_list))
    if not all(x==keys[0] for x in keys):
        sys.exit('Clone pair in different file')
    key = keys[0]
    start1 = min(map(lambda p: p[0].start, pair_list))
    end1 = max(map(lambda p: p[0].end, pair_list))
    start2 = min(map(lambda p: p[1].start, pair_list))
    end2 = max(map(lambda p: p[1].end, pair_list))
    return (Region(key[0], start1, end1), Region(key[1], start2, end2))


def merge_to_pairs(pair, pairs):
    pair = normalize_pair(pair)
    key = get_key(pair)
    if not key in pairs:
        pairs[key] = [pair]
    else:
        pair_list = pairs[key]
        all_contacted_pairs = contacted_pairs(pair, pair_list)
        for p in all_contacted_pairs:
            pair_list.remove(p)
        all_contacted_pairs.append(pair)
        merged = merge_contacted_pair_list(all_contacted_pairs)
        pair_list.append(merged)
        pairs[key] = pair_list



parser = argparse.ArgumentParser(description='Generate non-overlapping clone pairs in the format usable for BigCloneBench')
parser.add_argument('-o', '--output', type=argparse.FileType('w', encoding="ascii", errors="surrogateescape"), default=sys.stdout,
                    help='output json file')
parser.add_argument('-i', '--input', type=argparse.FileType('r', encoding="ascii", errors="surrogateescape"), default=sys.stdin,
                    help='input json file')
parser.add_argument('-R', '--maxrepeat', type=int, default=0, help='Maximal clone class size')
parser.add_argument('-G', '--gap', type=int, default=0, help='Allowed gap')
parser.add_argument('--bigclonebench', action='store_true')
parser.add_argument('-m', '--minlines', type=int, default=1, help='Minimal line numbers of clone pairs')

args = parser.parse_args()

pairs = {}
for line in tqdm(args.input):
    ob = json.loads(line)
    repeats = ob['locations']
    if args.maxrepeat ==0 or len(repeats) <= args.maxrepeat:
        for i in range(len(repeats)):
            for j in range(i+1, len(repeats)):
                pair = (repeats[i], repeats[j])
                merge_to_pairs(pair, pairs)

for _, pairs in pairs.items():
    for pair in pairs:
        if pair[0].end - pair[0].start >= args.minlines and pair[1].end - pair[1].start >= args.minlines:
            output_pair(pair)
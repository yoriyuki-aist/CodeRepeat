import argparse
import json
import os
import io
import sys
from collections import namedtuple
from tqdm import tqdm

input_stream = io.TextIOWrapper(sys.stdin.buffer, encoding='utf-8', errors='backslashreplace')
output_stream = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='backslashreplace')
err_stream = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8', errors='backslashreplace')

def read_lines_from_file(filename, start_line, end_line):
    lines = []

    with open(filename, 'r') as file:
        for line_number, line in enumerate(file, start=1):
            if start_line <= line_number <= end_line:
                lines.append(line)
            elif line_number > end_line:
                break

    return "".join(lines)


parser = argparse.ArgumentParser(description='vallidate the output')
parser.add_argument('-o', '--output', type=argparse.FileType('w', encoding="ascii", errors='backslashreplace'), default=output_stream,
                    help='output invalid json file')
parser.add_argument('-u', '--unprocessed', type=argparse.FileType('w', encoding="ascii", errors='backslashreplace'), default=err_stream,
                    help='output unprocessed json file')
parser.add_argument('-i', '--input', type=argparse.FileType('r', encoding="ascii", errors='backslashreplace'), default=input_stream,
                    help='input json file')
args = parser.parse_args()

for line in tqdm(args.input):
    try:
        invalid = None
        ob = json.loads(line)
        repeats = ob['locations']
        for repeat in repeats:
            text_in_file = read_lines_from_file(repeat['path'], 
                                                repeat['start_line'],
                                                repeat['end_line'])
            if not ob['text'] in text_in_file:
                invalid = ob
                break
        if not invalid is None:
            json.dump(ob, args.output)
            args.output.write('\n')
    except:
        args.unprocessed.write(f"{line} caused an error\n")
        continue
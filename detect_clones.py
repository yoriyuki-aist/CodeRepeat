#!/usr/bin/env python3
import argparse
import os
import subprocess

from typing import List


def unsigned_int(x):
    x = int(x)
    if x < 0:
        raise argparse.ArgumentTypeError("Value must be a positive integer")
    return x


def run(cmd: List[str]):
    print("Running '" + " ".join(cmd) + "'...")
    subprocess.check_call(cmd)


def run_preprocessor():
    pre_args = [
        "{}/bin/preprocessor".format(args.prefix),
        args.src,
        "{}.concat".format(intermediary),
        "{}.charmap".format(intermediary)
    ]
    if args.extensions:
        pre_args.extend(['--extensions', *args.extensions])
    run(pre_args)
    run([
        "{}/bin/bwt".format(args.prefix),
        "{}.concat".format(intermediary),
        "{}.bwt".format(intermediary)
    ])
    run([
        "{}/bin/converter".format(args.prefix),
        "{}.bwt".format(intermediary)
    ])


def run_findmaxrep():
    run([
        "{}/bin/findmaxrep".format(args.prefix),
        "-i", "{}.bwtraw".format(intermediary),
        "-P", "{}.bwtpos".format(intermediary),
        "-m", str(args.minrepeat)
    ])


def run_postprocessor():
    run([
        "{}/bin/postprocessor".format(args.prefix),
        "{}.bwtraw.output".format(intermediary),
        "{}.charmap".format(intermediary),
        output.name,
        "-m", str(args.minrepeat)
    ])


def parse_args():
    parser = argparse.ArgumentParser(description="Detects clones in a set of files")
    parser.add_argument('prefix', help='Location of the cmake build directory for the project')
    parser.add_argument('src', help='Input source directory to scan')
    parser.add_argument('-o', '--output', type=argparse.FileType('wb'), help='Output binary file (default: <src>.out)')
    parser.add_argument('-e', '--extensions', nargs='*',
                        help='List of file extensions to process (default: process all files)')
    parser.add_argument('-r', '--run', nargs='+', default='all', choices=['pre', 'findrepeats', 'post', 'all'],
                        help='List of steps to run')
    parser.add_argument('-m', '--min-repeat-length', dest="minrepeat", type=unsigned_int, default=10)
    parser.add_argument('--intermediaries',
                        help='Output directory for intermediary files (default: regular output directory)')
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()

    if not os.path.exists(args.src):
        raise argparse.ArgumentError("{} does not exist".format(args.src))

    output = args.output or open(args.src + ".out", "wb")

    if args.intermediaries:
        intermediary = "{}/{}".format(args.intermediaries, os.path.basename(args.src))
        if not os.path.exists(args.intermediaries):
            os.mkdir(args.intermediaries)
    else:
        intermediary = "{}/{}".format(os.path.dirname(output.name), os.path.basename(args.src))

    run_all = "all" in args.run

    if run_all or "pre" in args.run:
        run_preprocessor()

    if run_all or "findrepeats" in args.run:
        run_findmaxrep()

    if run_all or "post" in args.run:
        run_postprocessor()

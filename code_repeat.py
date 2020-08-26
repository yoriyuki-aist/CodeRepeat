#!/usr/bin/env python3
import argparse
import os
import shlex
import subprocess

from typing import List


def unsigned_int(x):
    x = int(x)
    if x < 0:
        raise argparse.ArgumentTypeError("Value must be a positive integer")
    return x


def run(cmd: List[str]):
    print("Running '" + " ".join(cmd) + "'...")
    cmd.insert(0, '/usr/bin/time')
    subprocess.check_call(cmd)


def run_preprocessor(args, intermediary):
    pre_args = [
        "{}/bin/preprocessor".format(args.prefix),
        args.src,
        "{}.concat".format(intermediary),
        "{}.charmap".format(intermediary),
        "--linemap", "{}.linemap".format(intermediary)
    ]
    if args.extensions:
        pre_args.extend(['--extensions', *args.extensions])
    if args.nl:
        pre_args.append('-nl')
    if args.nl2s:
        pre_args.append('-nl2s')
    if args.ns:
        pre_args.append('-ns')
    if args.ntr:
        pre_args.append('-ntr')
    run(pre_args)


def run_findrepset(args, intermediary):
    base_cmd = [
        "{}/bin/findrepset".format(args.prefix),
        "-ml", str(args.minrepeat),
    ]
    if not args.supermax:
        base_cmd.append("-nm"),  # find maximal repeats, not supermaximal ones
    concat_in = "{}.concat".format(intermediary)
    if args.compress:
        base_cmd.append(concat_in)
        cmd = " ".join([shlex.quote(c) for c in base_cmd]) + " -o /dev/fd/1 | gzip -c > " + shlex.quote(
            "{}.output.txt.gz".format(intermediary))
        print("Running '" + cmd + "'...")
        subprocess.run(cmd, shell=True, check=True)
    else:
        run([*base_cmd, "-o", "{}.output.txt".format(intermediary), concat_in])


def run_postprocessor(args, intermediary, output):
    post_args = [
        "{}/bin/postprocessor".format(args.prefix),
        ("{}.output.txt.gz" if args.compress else "{}.output.txt").format(intermediary),
        "{}.charmap".format(intermediary),
        "{}.linemap".format(intermediary),
        output.name,
        "-m", str(args.minrepeat)
    ]
    if args.skip_blank:
        post_args.append('--skip-blank')
    if args.skip_null:
        post_args.append('--skip-null')
    if args.compress:
        post_args.append('--compress')
    run(post_args)


def run_scan(args):
    if not os.path.exists(args.src):
        raise argparse.ArgumentError("{} does not exist".format(args.src))

    output = args.output or open(args.src + ".json" + (".gz" if args.compress else ""), "wb")

    if args.intermediaries:
        intermediary = "{}/{}".format(args.intermediaries, os.path.basename(args.src))
        if not os.path.exists(args.intermediaries):
            os.mkdir(args.intermediaries)
    else:
        intermediary = "{}/{}".format(os.path.dirname(output.name), os.path.basename(args.src))

    run_all = "all" in args.run

    if run_all or "pre" in args.run:
        run_preprocessor(args, intermediary)

    if run_all or "findrepeats" in args.run:
        run_findrepset(args, intermediary)

    if run_all or "post" in args.run:
        run_postprocessor(args, intermediary, output)


def parse_args():
    parser = argparse.ArgumentParser(description="Scan a source repository for clones and output a JSON description")
    parser.add_argument('--prefix', help='Location of the cmake build directory for the project',
                        default=os.path.dirname(__file__))
    parser.add_argument('-z', '--compress', dest='compress', action='store_true',
                        help='Use compressed intermediate and json files')
    parser.add_argument('src', help='Input source directory to scan')
    parser.add_argument('-o', '--output', type=argparse.FileType('w'),
                             help='Output JSON file (default: <src>.json)')
    parser.add_argument('-e', '--extensions', nargs='*',
                             help='List of file extensions to process (default: process all files)')
    parser.add_argument('-r', '--run', nargs='+', default='all', choices=['pre', 'findrepeats', 'post', 'all'],
                             help='List of steps to run')
    parser.add_argument('-m', '--min-repeat-length', dest="minrepeat", type=unsigned_int, default=10,
                             help='Minimum size of the repeated sequences')
    parser.add_argument('-i', '--intermediaries',
                             help='Output directory for intermediary files (default: regular output directory)')
    pre_group = parser.add_argument_group('Pre-processing', 'Options for the "pre" step. '
                                                                 'Space-producing transformations are applied before '
                                                                 'space normalization.')
    pre_group.add_argument('--normalize-newlines', dest='nl', action='store_true',
                           help='Remove carriage returns (\\r) from the text (default: false)')
    pre_group.add_argument('--newlines-to-spaces', dest='nl2s', action='store_true',
                           help='Replace carriage returns and line feeds with common spaces (default: false)')
    pre_group.add_argument('--normalize-spaces', dest='ns', action='store_true',
                           help='Replace sequences of whitespace with a single common space (default: false)')
    pre_group.add_argument('--normalize-trailing', dest='ntr', action='store_true',
                           help='Truncate sequences of whitespace preceding a line feed (default: false)')
    find_group = parser.add_argument_group('Repeat Finding', 'Options for the "findmaxrep" step.')
    find_group.add_argument('--supermax', action='store_true', help='Use supermaximal repeats')
    post_group = parser.add_argument_group('Post-processing', 'Options for the "post" step')
    post_group.add_argument('--skip-blank', dest='skip_blank', action='store_true',
                            help='Skip repeated sequences that only contain whitespace and control code'
                                 '(default: false)')
    post_group.add_argument('--skip-null', dest='skip_null', action='store_true',
                            help='Skip repeated sequences that only contain null (default: false)')
    parser.set_defaults(launch=run_scan)

    return parser.parse_args()


def main():
    run_scan(parse_args())


if __name__ == "__main__":
    main()

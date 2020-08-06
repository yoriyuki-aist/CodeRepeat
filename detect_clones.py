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
    cmd.insert(0, '/usr/bin/time')
    subprocess.check_call(cmd)


def run_preprocessor(args, intermediary):
    pre_args = [
        "{}/bin/preprocessor".format(args.prefix),
        args.src,
        "{}.concat".format(intermediary),
        "{}.charmap".format(intermediary)
    ]
    if args.extensions:
        pre_args.extend(['--extensions', *args.extensions])
    if args.linemap:
        pre_args.extend(['--linemap', '{}.linemap'.format(intermediary)])
    if args.nl:
        pre_args.append('-nl')
    if args.nl2s:
        pre_args.append('-nl2s')
    if args.ns:
        pre_args.append('-ns')
    if args.ntr:
        pre_args.append('-ntr')
    run(pre_args)


def run_findmaxrep(args, intermediary):
    run([
        "{}/bin/bwt".format(args.prefix),
        "{}.concat".format(intermediary),
        "{}.bwt".format(intermediary)
    ])
    run([
        "{}/bin/converter".format(args.prefix),
        "{}.bwt".format(intermediary)
    ])
    run([
        "{}/bin/findmaxrep".format(args.prefix),
        "-i", "{}.bwtraw".format(intermediary),
        "-P", "{}.bwtpos".format(intermediary),
        "-o", "{}.output.txt".format(intermediary),
        "-m", str(args.minrepeat)
    ])


def run_findrepset(args, intermediary):
    run([
        "{}/bin/findrepset".format(args.prefix),
        "-nm",  # find maximal repeats, not supermaximal ones
        "-ml", str(args.minrepeat),
        "-o", "{}.output.txt".format(intermediary),
        "{}.concat".format(intermediary)
    ])


def run_postprocessor(args, intermediary, output):
    post_args = [
        "{}/bin/postprocessor".format(args.prefix),
        "{}.output.txt".format(intermediary),
        "{}.charmap".format(intermediary),
        output.name,
        "-m", str(args.minrepeat)
    ]
    if args.linemap:
        post_args.extend(['--linemap', '{}.linemap'.format(intermediary)])
    if args.skip_blank:
        post_args.append('--skip-blank')
    if args.skip_null:
        post_args.append('--skip-null')
    run(post_args)


def run_scan(args):
    if not os.path.exists(args.src):
        raise argparse.ArgumentError("{} does not exist".format(args.src))

    output = args.output or open(args.src + ".json", "wb")

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
        if args.alt_finder:
            run_findmaxrep(args, intermediary)
        else:
            run_findrepset(args, intermediary)

    if run_all or "post" in args.run:
        run_postprocessor(args, intermediary, output)


def run_stats(args):
    stats_args = [
        "{}/bin/clonestats".format(args.prefix),
        args.input.name
    ]

    if args.output:
        stats_args.extend(['-o', args.output.name])

    if args.count:
        stats_args.append('--count')

    if args.distance:
        stats_args.append('--distance')

    if args.bigcloneeval:
        stats_args.append('--bigcloneeval')

    if args.connectivity:
        stats_args.extend(['--connectivity', args.connectivity.name])

    run(stats_args)


def parse_args():
    parser = argparse.ArgumentParser(description="Detects clones in a set of files")
    parser.add_argument('--prefix', help='Location of the cmake build directory for the project',
                        default=os.path.dirname(__file__))
    subparsers = parser.add_subparsers(dest='cmd', metavar='{scan,stats}', required=True)
    scan_parser = subparsers.add_parser('scan',
                                        help='Scan a source repository for clones and output a JSON description')
    scan_parser.add_argument('src', help='Input source directory to scan')
    scan_parser.add_argument('-o', '--output', type=argparse.FileType('w'),
                             help='Output JSON file (default: <src>.json)')
    scan_parser.add_argument('-e', '--extensions', nargs='*',
                             help='List of file extensions to process (default: process all files)')
    scan_parser.add_argument('-r', '--run', nargs='+', default='all', choices=['pre', 'findrepeats', 'post', 'all'],
                             help='List of steps to run')
    scan_parser.add_argument('-m', '--min-repeat-length', dest="minrepeat", type=unsigned_int, default=10,
                             help='Minimum size of the repeated sequences')
    scan_parser.add_argument('-i', '--intermediaries',
                             help='Output directory for intermediary files (default: regular output directory)')
    scan_parser.add_argument('--linemap', action='store_true',
                           help='Export mappings from character position to line number')
    pre_group = scan_parser.add_argument_group('Pre-processing', 'Options for the "pre" step. '
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
    find_group = scan_parser.add_argument_group('Repeat Finding', 'Options for the "findmaxrep" step.')
    find_group.add_argument('--alt-finder', dest='alt_finder', action='store_true',
                            help='Use the alternative (slower) repeat finder (default: false)')
    post_group = scan_parser.add_argument_group('Post-processing', 'Options for the "post" step')
    post_group.add_argument('--skip-blank', dest='skip_blank', action='store_true',
                            help='Skip repeated sequences that only contain whitespace and control code (default: false)')
    post_group.add_argument('--skip-null', dest='skip_null', action='store_true',
                            help='Skip repeated sequences that only contain null (default: false)')
    scan_parser.set_defaults(launch=run_scan)
    stat_parser = subparsers.add_parser('stats')
    stat_parser.add_argument('input', type=argparse.FileType('r'), help='JSON file emitted by the scan process')
    stat_parser.add_argument('-o', '--output', type=argparse.FileType('w'), help='Output CSV file (default: stdout)')
    stat_parser.add_argument('--count', dest='count', action='store_true', help='Create a count matrix')
    stat_parser.add_argument('--distance', action='store_true', help='Create a distance matrix')
    stat_parser.add_argument('--connectivity', type=argparse.FileType('w'), help='Saves a connectivity matrix to '
                                                                                 'the given file')
    stat_parser.add_argument('--bigcloneeval', action='store_true', help='Export CSV file with the BigCloneEval format')
    stat_parser.set_defaults(launch=run_stats)

    return parser.parse_args()


def main():
    args = parse_args()
    args.launch(args)


if __name__ == "__main__":
    main()

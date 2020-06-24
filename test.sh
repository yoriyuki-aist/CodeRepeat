if [ -z $1 ]; then
  echo "Usage: $0 <src> [<destfolder>]"
  echo "Run '$0 -h' for more detailed instructions"
  exit 1
fi

if [[ "$1" == "-h" || "$1" == "--help" ]]; then
  echo "
Execution script for the clone detection tool
Usage: $0 <src> [<destfolder>]

If no destination folder is specified, output files get written to the parent of the source folder.

Environment variables:
  - \$PREFIX: location of the cmake build directory for the project. Must be specified.
  - \$EXTENSIONS: list of file extension_filter to process, separated by spaces. Defaults to processing every file.
  - \$INTERMEDIARIES: output directory for intermediary files. Defaults to <destfolder>.
  - \$MIN_REPEAT_LENGTH: minimum length of processed repeated sequences. Defaults to 10.
"
  exit 0
fi

if [ -z "$PREFIX" ]; then
  echo "The PREFIX environment variable must be set to the location of the cmake build directory."
  exit 1
fi

if [ -z "$EXTENSIONS" ]; then
  read -rp "Empty EXTENSIONS environment variable, every file in $src will be processed. Press any key to proceed.
"
  extension_filter=""
else
  extension_filter="--extensions $EXTENSIONS"
fi

src="${1%/}" # remove trailing slash
if [ -z "$2" ]; then
  dest=$(dirname "$src")
else
  dest="${2%/}"
  mkdir -p "$dest"
fi

if [ -z "$INTERMEDIARIES" ]; then
  intermediary="$dest/$(basename "$src")"
else
  intermediary="${INTERMEDIARIES%/}/$(basename "$src")"
  mkdir -p "$INTERMEDIARIES"
fi

if [ -z "$MIN_REPEAT_LENGTH" ]; then
  minlength=10
else
  case "$MIN_REPEAT_LENGTH" in
      ''|*[!0-9]*) echo "MIN_REPEAT_LENGTH environment variable must be numeric, was $MIN_REPEAT_LENGTH."; exit 1 ;;
      *) minlength="$MIN_REPEAT_LENGTH" ;;
  esac
fi

# EXTENSIONS is deliberately expanded
# shellcheck disable=SC2086
"$PREFIX"/bin/preprocessor "$src" "$intermediary".concat "$intermediary".charmap $extension_filter &&\
  "$PREFIX"/bin/bwt "$intermediary".concat "$intermediary".bwt &&\
  "$PREFIX"/bin/converter "$intermediary".bwt &&\
  "$PREFIX"/bin/findmaxrep -i "$intermediary.bwtraw" -P "$intermediary.bwtpos" -m $minlength &&\
  "$PREFIX"/bin/postprocessor "$intermediary.bwtraw.output" "$intermediary.charmap" "$dest/$(basename $src).json"

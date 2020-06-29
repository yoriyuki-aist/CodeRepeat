if [ -z $1 ]; then
  echo "Usage: $0 <pre|findrepeats|post|all|help>"
  exit 1
fi

shift;

if [[ "$1" == "help" || "$1" == "-h" ]]; then
  echo "
Execution script for the clone detection tool
Usage: $0 <pre|findrepeats|post|all|help>

If no destination folder is specified, output files get written to the parent of the source folder.

Arguments:
  -i <dir>: input directory for the script. Must be specified.
  --prefix <dir>: location of the cmake build directory for the project. Must be specified.
  --extensions <exts...>: quoted list of file extensions to process, separated by spaces. Defaults to processing every file.
  --intermediaries <dir>: output directory for intermediary files. Defaults to <destfolder>.
  --min-repeat-length <nb>: minimum length of processed repeated sequences. Defaults to 10.
"
  exit 0
fi

while true; do
  case "$1" in
    -i )
      src="${2%/}"; shift 2 ;;
    --prefix )
      PREFIX="$2"; shift 2 ;;
    --extensions )
      EXTENSIONS="$2"; shift 2 ;;
    --intermediaries ) INTERMEDIARIES="$2"; shift 2 ;;
    --min-repeat-length ) MIN_REPEAT_LENGTH="$2"; shift 2 ;;
    -- ) shift; break ;;
    * ) break ;;
  esac
done

if [ -z "$src" ]; then
  echo "The -i argument must be set to the location of the input directory."
  exit 1
fi

if [ -z "$PREFIX" ]; then
  echo "The --prefix argument must be set to the location of the cmake build directory."
  exit 1
fi

if [ -z "$EXTENSIONS" ]; then
  read -rp "Empty --extensions argument, every file in $src will be processed. Press any key to proceed.
"
  extension_filter=""
else
  extension_filter="--extensions $EXTENSIONS"
fi

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

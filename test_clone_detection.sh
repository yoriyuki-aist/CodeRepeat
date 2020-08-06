src=$(dirname "$(realpath "$0")")
tmpdir=/tmp/test_clone_detection/
mkdir -p "$tmpdir"
"$src"/detect_clones.py "--prefix=$src/cmake-build-debug" scan "$1" --linemap -o "$tmpdir/test.json" >& /dev/null || exit 1
"$src"/detect_clones.py "--prefix=$src/cmake-build-debug" stats "$tmpdir/test.json" --bigcloneeval -o "$tmpdir/out.csv" >& /dev/null || exit 1
cat "$tmpdir/out.csv"

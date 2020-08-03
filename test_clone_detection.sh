src=$(dirname "$(realpath "$0")")
tmpdir=/tmp/test_clone_detection/
mkdir -p "$tmpdir"
"$src"/detect_clones.py "--prefix=$src/cmake-build-debug" scan "$1" --linemap "$tmpdir/linemap" -o "$tmpdir/test.json" >& /dev/null &&\
"$src"/detect_clones.py "--prefix=$src/cmake-build-debug" stats "$tmpdir/test.json" -o "$tmpdir/out.csv" >& /dev/null &&\
cat "$tmpdir/out.csv"

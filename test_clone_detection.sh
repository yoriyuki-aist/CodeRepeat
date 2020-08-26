src=$(dirname "$(realpath "$0")")
tmpdir=/home/yoriyuki/tmp/test_clone_detection
mkdir -p "$tmpdir"
#rm $tmpdir/*
"$src"/code_repeat.py "--prefix=$src" -z -m 240 -e java --normalize-newlines --newlines-to-spaces --normalize-spaces --normalize-trailing --skip-blank  -o "$tmpdir/test.json.gz"  "$1" >> "$tmpdir/out.log" 2>> "$tmpdir/err.log" || exit 1
gunzip -c "$tmpdir/test.json.gz" | "$src"/scripts/clonepairs.py -R 100 -G 0 -o "$tmpdir/out.csv" > "$tmpdir/out.log" 2> "$tmpdir/err.log" || exit 2
cat "$tmpdir/out.csv"

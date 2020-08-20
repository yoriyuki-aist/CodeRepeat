src=$(dirname "$(realpath "$0")")
tmpdir=/home/yoriyuki/tmp/test_clone_detection
mkdir -p "$tmpdir"
rm $tmpdir/*
"$src"/detect_clones.py "--prefix=$src" -z scan "$1" -m 40 -e java --linemap --supermax --normalize-newlines --newlines-to-spaces --normalize-spaces --normalize-trailing --skip-blank --verbose -o "$tmpdir/test.json.gz" >& /dev/null || exit 1
gunzip -c "$tmpdir/test.json.gz" | "$src"/bigcloneeval.py -o "$tmpdir/out.csv" > "$tmpdir/out.log" 2> "$tmpdir/err.log" || exit 2
cat "$tmpdir/out.csv"

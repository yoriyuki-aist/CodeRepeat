if [ -z $1 ];
then
	echo "Usage: $0 <srcfolder>"
	exit 1
fi
if [ -z $PREFIX ]
then
  echo "The PREFIX environment variable must be set to the location of the cmake build directory."
  exit 1
fi
$PREFIX/bin/preprocessor $1 $1.concat $1.charmap --extensions .rs .cpp .h .hpp .java .py .js .txt
$PREFIX/bin/bwt $1.concat $1.bwt
$PREFIX/bin/converter $1.bwt
case "$OSTYPE" in
  darwin*)  TIMECOM=/usr/local/bin/gtime ;; 
  *)        TIMECOM=/usr/bin/time;;
esac
start="$(date --iso-8601=ns)"
$TIMECOM -v -o "$1.fmrtime" $PREFIX/bin/findmaxrep -i $1.bwtraw -P $1.bwtpos -m 10
end="$(date --iso-8601=ns)"
$PREFIX/bin/postprocessor $1.bwtraw.output $1.charmap
elapsed="
start: $start
end: $end
"
echo "$elapsed" >> "$1.fmrtime"
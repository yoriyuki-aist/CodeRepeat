if [ -z $1 ];
then
	echo "Usage: $0 <srcfolder>"
	exit
fi
if [ -z $PREFIX ]
then
  echo "The PREFIX environment variable must be set to the location of the cmake build directory."
fi
$PREFIX/bin/preprocessor $1 $1.concat
$PREFIX/bin/bwt $1.concat $1.bwt
$PREFIX/bin/converter $1.bwt
case "$OSTYPE" in
  darwin*)  TIMECOM=/usr/local/bin/gtime ;; 
  *)        TIMECOM=/usr/bin/time;;
esac
$TIMECOM -v -o "$1.fmrtime" $PREFIX/bin/findmaxrep -i $1.bwtraw -P $1.bwtpos -m 100

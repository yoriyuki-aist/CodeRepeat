if [ $# -ne 1 ];
then
	echo "Usage: $0 <srcfolder>"
	exit
fi
cmake-build-debug/bin/preprocessor $1 $1.concat
cmake-build-debug/bin/bwt $1.concat $1.bwt
cmake-build-debug/bin/converter $1.bwt
/usr/bin/time -v -o "$1.fmrtime" cmake-build-debug/bin/findmaxrep -i $1.bwtraw -P $1.bwtpos -m 100

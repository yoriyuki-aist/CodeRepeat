loc=$(realpath $1)
origin=$(pwd)
cd "$(dirname $0)" || exit
./detect_clones.py '--prefix=cmake-build-debug' scan "$loc" -o '/tmp/test.json' >& /dev/null &&\
./detect_clones.py '--prefix=cmake-build-debug' stats '/tmp/test.json' >& /dev/null
cd "$origin" || exit
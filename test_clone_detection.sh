loc=$(realpath $1)
origin=$(pwd)
cd "$(dirname $0)" || exit
pwd
ls "$loc"
./detect_clones.py '--prefix=cmake_build_debug' scan "$loc" -o 'test.json' > /dev/null &&\
./detect_clones.py '--prefix=cmake_build_debug' stats 'test.json'
cd "$origin" || exit
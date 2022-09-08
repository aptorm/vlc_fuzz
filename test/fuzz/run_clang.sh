if test -z "$1"; then
	echo "Usage: $0 <target>"
	echo "Example: $0 libvlc_demux_fuzzer"
	exit 1
fi

target=$1
workers=1

CXXFLAGS="-O0 -fno-omit-frame-pointer -gline-tables-only -fsanitize=fuzzer,address -fsanitize-address-use-after-scope -fsanitize-coverage=trace-pc-guard,trace-cmp -stdlib=libstdc++"
LD_LIBRARY_PATH=../../src/.libs/:../../lib/.libs

clang++ \
    ${CXXFLAGS} -I../../include/ -I../../include/vlc \
    ${target}.cpp -o ${target} \
    -L../../src/.libs/ -lvlccore -L../../lib/.libs -lvlc -L./ -lFuzzer \
    -std=c++11

export LD_LIBRARY_PATH
export ASAN_SYMBOLIZER_PATH=/usr/bin/llvm-symbolizer-14

./${target} -workers=$workers DEMUX_CORPUS 

exit 0

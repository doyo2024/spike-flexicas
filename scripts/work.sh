#! /bin/sh

# if [ -z "$TOP" ]; then 
#   export  TOP=/home/spike/Desktop/Riscv
#   export  RISCV=$TOP/riscv-tools/riscv-tc
#   export  PATH=$PATH:$RISCV/bin

#   # export    LD_LIBRARY_PATH=/home/spike/Desktop/spike-flexicas/build 
# else 
#   echo "TOP has been set."
# fi

. scripts/env.sh

# echo "TOP=${TOP}"
echo "RISCV=${RISCV}"

echo "now in $(pwd)"

cd trace_capture
make clean
make
cd ..

cd build
make clean
cp ../trace_capture/libtraceGen.a libtraceGen.a
make libflexicas.so
make -j100
cp spike $RISCV/bin/spike-flexicas-new

echo "finished!"
#! /bin/sh

# if [ -z "$TOP" ]; then 
#   export  TOP=/home/spike/Desktop/Riscv
#   export  RISCV=$TOP/riscv-tools/riscv-tc
#   export  PATH=$PATH:$RISCV/bin
#   export  LD_LIBRARY_PATH=/home/spike/Desktop/spike-flexicas/build 
# else 
#   echo "TOP has been set."
# fi

. scripts/env.sh

spike-flexicas-new $RISCV/bin/bbl
#! /bin/sh

if [ -z "$RISCV" ]; then 
  # export  TOP=/home/spike/Desktop/Riscv
  # export  RISCV=$TOP/riscv-tools/riscv-tc
  export  RISCV=/home/yzx/yzx/Riscv
  export  PATH=$PATH:$RISCV/bin
  # export  LD_LIBRARY_PATH=/home/yzx/yzx/code/spike-flexicas/build
  export  ATTACK_DIR=/home/yzx/yzx/tools/spike-cache-attack
else 
  echo "RISCV has been set."
fi
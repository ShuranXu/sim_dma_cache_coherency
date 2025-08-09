#!/usr/bin/env bash
# install_spike.sh — Download & install Spike and proxy-kernel

set -euo pipefail


# Set installation prefix
export RISCV="${RISCV:-$HOME/riscv}"
mkdir -p "$RISCV"

# Build and install the RISC-V GNU toolchain (newlib)
if [ ! -d "${RISCV}/riscv-gnu-toolchain" ]; then
  sudo apt-get install -y texinfo
  git clone https://github.com/riscv/riscv-gnu-toolchain.git "$RISCV/riscv-gnu-toolchain"
  pushd "$RISCV/riscv-gnu-toolchain"
  ./configure --prefix="$RISCV" --enable-multilib
  make linux -j"$(nproc)"
  make install
  popd
fi
export PATH="$RISCV/bin:$PATH"

# Build and install Spike (ISA simulator)
if [ ! -d "${RISCV}/riscv-isa-sim" ]; then
  sudo apt-get install -y \
    libboost-dev \
    libboost-system-dev \
    libboost-thread-dev \
    libboost-program-options-dev
  git clone https://github.com/riscv/riscv-isa-sim.git "$RISCV/riscv-isa-sim"
  pushd "$RISCV/riscv-isa-sim"
  mkdir -p build && cd build
  ../configure --prefix="$RISCV"
  make -j"$(nproc)"
  make install
  popd
fi

# Build and install proxy-kernel (pk)
if [ ! -d "${RISCV}/riscv-pk" ]; then
  git clone https://github.com/riscv/riscv-pk.git "$RISCV/riscv-pk"
  pushd "$RISCV/riscv-pk"
  mkdir -p build && cd build
  ../configure --prefix="$RISCV" --host=riscv64-unknown-linux-gnu
  make -j"$(nproc)"
  make install
  popd
fi

echo "Installation complete. Add to your shell profile:"
echo "  export RISCV=$RISCV"
echo "  export PATH=\$RISCV/bin:\$PATH"

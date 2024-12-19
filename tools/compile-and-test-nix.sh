#!/usr/bin/env bash

#set -eu

root=$(git rev-parse --show-toplevel)
cd "${root}"

if [[ ${#} -gt 1 ]]; then
  compilers=(
    ${@}
  )
else
  compilers=(
    gcc9
    gcc10
    gcc11
    gcc12
    gcc13
    gcc14
  )
fi

for compiler in ${compilers[@]}; do
  NIX_SHELL_RUN="nix-shell etc/env/nix/shell.nix --argstr compiler ${compiler} --pure --run"
  echo "${NIX_SHELL_RUN}"
  echo "Testing compiler ${compiler}"
  ${NIX_SHELL_RUN} "make -sj 8 extern CONFIG=nix-${compiler}"
  ${NIX_SHELL_RUN} "make -sj 8 cc4s CONFIG=nix-${compiler}"
  ${NIX_SHELL_RUN} "make -sj 8 test-run CONFIG=nix-${compiler}"
  ${NIX_SHELL_RUN} "make -sj 8 test-check CONFIG=nix-${compiler}"
done

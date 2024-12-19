#!/usr/bin/env bash

set -eu

root=$(git rev-parse --show-toplevel)
cd "${root}"

if [[ ${#} -gt 0 ]]; then
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

cat <<EOF
Testing compilers
    ${compilers[@]}

and action
    ${CI_MAKE_RULE:-All makefile rules}
EOF

for compiler in ${compilers[@]}; do
  NIX_SHELL_RUN="nix-shell etc/env/nix/shell.nix --argstr compiler ${compiler} --pure --run"
  echo "${NIX_SHELL_RUN}"
  echo "Testing compiler ${compiler}"
  if [[ -z ${CI_MAKE_RULE} ]]; then
    ${NIX_SHELL_RUN} "make -sj 8 extern CONFIG=nix-${compiler}"
    ${NIX_SHELL_RUN} "make -sj 8 cc4s CONFIG=nix-${compiler}"
    ${NIX_SHELL_RUN} "make -sj 8 test-run CONFIG=nix-${compiler}"
    ${NIX_SHELL_RUN} "make -sj 8 test-check CONFIG=nix-${compiler}"
  else
    ${NIX_SHELL_RUN} "make -sj 8 ${CI_MAKE_RULE} CONFIG=nix-${compiler}"
  fi
done

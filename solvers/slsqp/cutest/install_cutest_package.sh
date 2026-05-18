#!/usr/bin/env bash
set -euo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ascend_root=$(cd "${script_dir}/../../.." && pwd)
cutest_root=${CUTEST:-/home/john/CUTEst}

mkdir -p "${cutest_root}/src/slsqp" "${cutest_root}/packages/defaults"
ln -sf "${script_dir}/slsqp_main.c" "${cutest_root}/src/slsqp/slsqp_main.c"
ln -sf "${script_dir}/makemaster" "${cutest_root}/src/slsqp/makemaster"
cp "${script_dir}/package_default_slsqp" "${cutest_root}/packages/defaults/slsqp"
chmod +x "${cutest_root}/packages/defaults/slsqp"

cat <<EOF
Installed CUTEst SLSQP package hooks:
  ${cutest_root}/src/slsqp/slsqp_main.c -> ${script_dir}/slsqp_main.c
  ${cutest_root}/src/slsqp/makemaster -> ${script_dir}/makemaster
  ${cutest_root}/packages/defaults/slsqp

Run example:
  export CUTEST=${cutest_root}
  export ASCEND_ROOT=${ascend_root}
  export LD_LIBRARY_PATH=${ascend_root}:\${LD_LIBRARY_PATH:-}
  runcutest -p slsqp -D HS21
EOF

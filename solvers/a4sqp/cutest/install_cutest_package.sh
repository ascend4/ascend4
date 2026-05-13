#!/usr/bin/env bash
set -euo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ascend_root=$(cd "${script_dir}/../../.." && pwd)
cutest_root=${CUTEST:-/home/john/CUTEst}

mkdir -p "${cutest_root}/src/a4sqp" "${cutest_root}/src/ipoptc" "${cutest_root}/packages/defaults"
ln -sf "${script_dir}/a4sqp_main.c" "${cutest_root}/src/a4sqp/a4sqp_main.c"
ln -sf "${script_dir}/makemaster" "${cutest_root}/src/a4sqp/makemaster"
cp "${script_dir}/package_default_a4sqp" "${cutest_root}/packages/defaults/a4sqp"
chmod +x "${cutest_root}/packages/defaults/a4sqp"
ln -sf "${script_dir}/ipoptc_main.c" "${cutest_root}/src/ipoptc/ipoptc_main.c"
ln -sf "${script_dir}/makemaster_ipoptc" "${cutest_root}/src/ipoptc/makemaster"
cp "${script_dir}/package_default_ipoptc" "${cutest_root}/packages/defaults/ipoptc"
chmod +x "${cutest_root}/packages/defaults/ipoptc"

cat <<EOF
Installed CUTEst A4SQP/IPOPTC package hooks:
  ${cutest_root}/src/a4sqp/a4sqp_main.c -> ${script_dir}/a4sqp_main.c
  ${cutest_root}/src/a4sqp/makemaster -> ${script_dir}/makemaster
  ${cutest_root}/packages/defaults/a4sqp
  ${cutest_root}/src/ipoptc/ipoptc_main.c -> ${script_dir}/ipoptc_main.c
  ${cutest_root}/src/ipoptc/makemaster -> ${script_dir}/makemaster_ipoptc
  ${cutest_root}/packages/defaults/ipoptc

Run example:
  export CUTEST=${cutest_root}
  export ASCEND_ROOT=${ascend_root}
  export LD_LIBRARY_PATH=${ascend_root}/solvers/a4sqp:${ascend_root}:\${LD_LIBRARY_PATH:-}
  runcutest -p a4sqp -D HS11
  runcutest -p ipoptc -D HS11
EOF

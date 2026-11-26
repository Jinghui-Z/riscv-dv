#!/usr/bin/env bash
set -euo pipefail

repo_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
cd "$repo_dir"

test_name=nanhu_big_random_irq_nmi_forward_progress
testlist=yaml/big_random_feature_matrix.yaml
python_bin=${PYTHON:-python3}
preflight=${DV_WORKFLOW_SKILL_HOME:-$HOME/.codex/skills/dv-workflow}/scripts/dv_preflight.py
iterations=${ITERATIONS:-1000}
start_seed=${START_SEED:-20261111}
iss_timeout=${ISS_TIMEOUT:-600}
cmp_opts=${CMP_OPTS:-+define+NANHU_V5_1_SV48}
output=${OUT:-out/${test_name}_seed${start_seed}_$(date +%Y%m%d_%H%M%S)}

if [[ -e "$output" ]]; then
  printf 'Output already exists: %s\n' "$output" >&2
  exit 1
fi

"$python_bin" "$preflight" --repo "$repo_dir" --target nanhu_v5_1 \
  --testlist "$testlist" --test "$test_name" --simulator vcs \
  --cmp-opts "$cmp_opts" --steps gen,gcc_compile,iss_sim --iss spike \
  --iss-timeout "$iss_timeout" --iterations "$iterations" --batch_size 1 \
  --start_seed "$start_seed" --output "$output"

"$python_bin" run.py --target nanhu_v5_1 --testlist "$testlist" \
  --test "$test_name" --simulator vcs --cmp_opts "$cmp_opts" \
  --steps gen,gcc_compile,iss_sim --iss spike --gen_timeout 3600 \
  --iss_timeout "$iss_timeout" --iterations "$iterations" --batch_size 1 \
  --start_seed "$start_seed" --output "$output"

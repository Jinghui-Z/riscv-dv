#!/usr/bin/env bash
set -euo pipefail

cd "$HOME/tool/ISG/riscv-dv"
TL=yaml/big_random_feature_matrix.yaml
PF="$HOME/.codex/skills/dv-workflow/scripts/dv_preflight.py"
# The default draws from the individually Spike-validated six-case pool.
# Set TEST=nanhu_big_random_hypertest_branch_mix to explore the full pool, or
# TEST=nanhu_big_random_hypertest to require every injection to execute.
TEST="${TEST:-nanhu_big_random_hypertest_branch_mix_reviewed}"
ITERATIONS="${ITERATIONS:-500}"
ISS_TIMEOUT="${ISS_TIMEOUT:-600}"
START_SEED="${START_SEED:-20261109}"
OUT="${OUT:-out/${TEST}_seed${START_SEED}_$(date +%Y%m%d_%H%M%S)}"
CMP='+define+NANHU_V5_1_SV48'

if [[ -e "$OUT" ]]; then
  printf 'Output already exists: %s\n' "$OUT" >&2
  exit 1
fi

python3 "$PF" --repo "$PWD" --target nanhu_v5_1 --testlist "$TL" \
  --test "$TEST" --simulator vcs --cmp-opts "$CMP" \
  --steps gen,gcc_compile,iss_sim --iss spike \
  --iss-timeout "$ISS_TIMEOUT" --iterations "$ITERATIONS" --batch_size 1 \
  --start_seed "$START_SEED" --output "$OUT"

python3 run.py --target nanhu_v5_1 --testlist "$TL" --test "$TEST" \
  --simulator vcs --cmp_opts "$CMP" --steps gen,gcc_compile,iss_sim \
  --iss spike --gen_timeout 3600 --iss_timeout "$ISS_TIMEOUT" \
  --iterations "$ITERATIONS" --batch_size 1 --start_seed "$START_SEED" --output "$OUT"

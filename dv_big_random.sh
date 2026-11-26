cd "$HOME/tool/ISG/riscv-dv"
TL=yaml/big_random_feature_matrix.yaml
PF="$HOME/.codex/skills/dv-workflow/scripts/dv_preflight.py"
TEST=nanhu_big_random_boot_directed
OUT=out/nanhu_100k_boot_directed_20261104

python3 "$PF" --repo "$PWD" --target nanhu_v5_1 --testlist "$TL" \
  --test "$TEST" --simulator vcs --steps gen,gcc_compile,iss_sim \
  --iss spike --iss-timeout 600 --iterations 10 --batch_size 1 --start_seed 20261104 --output "$OUT"

python3 run.py --target nanhu_v5_1 --testlist "$TL" --test "$TEST" \
  --simulator vcs --steps gen,gcc_compile,iss_sim --iss spike \
  --gen_timeout 3600 --iss_timeout 600 --iterations 10 --batch_size 1 --start_seed 20261104 --output "$OUT"
#!/usr/bin/env bash
set -euo pipefail

# orion_bench_smoke.sh
# Fast, representative performance smoke test for Orion
# - Pins to a single core to reduce scheduler noise
# - Uses fewer repetitions and shorter min_time for quick signal
# - Focuses on representative benchmarks (can be overridden)

CORE=${CORE:-2}
REPS=${REPS:-5}
MINTIME=${MINTIME:-20ms}
FILTER=${FILTER:-BM_CalcDiscount}
BINARY=${BINARY:-./build/orion-bench}

if [[ ! -x "$BINARY" ]]; then
  echo "Error: benchmark binary not found at $BINARY" >&2
  exit 1
fi

# Show effective parameters
echo "[orion_bench_smoke] core=$CORE reps=$REPS min_time=$MINTIME filter=$FILTER binary=$BINARY"

# Pin and run
exec taskset -c "$CORE" "$BINARY" \
  --benchmark_repetitions="$REPS" \
  --benchmark_min_time="$MINTIME" \
  --benchmark_report_aggregates_only=true \
  --benchmark_filter="$FILTER"

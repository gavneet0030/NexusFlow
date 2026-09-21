import csv
import os
import statistics

root = r"C:\Users\PC\NexusFlow"

v1_path = os.path.join(
    root,
    "benchmarks",
    "results",
    "real_end_to_end_latency_multi_load.csv"
)

v2_path = os.path.join(
    root,
    "benchmarks",
    "results",
    "real_end_to_end_latency_multi_load_v2_20260908_185246.csv"
)

def read_csv(path):
    with open(path, newline="", encoding="utf-8-sig") as f:
        rows = list(csv.DictReader(f))

    print(f"Loaded {len(rows)} rows from:")
    print(path)

    return rows


def find_column(row, names):
    lowered = {k.lower().strip(): k for k in row.keys()}

    for name in names:
        if name.lower() in lowered:
            return lowered[name.lower()]

    return None


def aggregate(rows):
    if not rows:
        return {}

    sample = rows[0]

    rate_col = find_column(
        sample,
        [
            "target_rate_eps",
            "target_rate",
            "offered_rate_eps",
            "load_eps",
            "rate"
        ]
    )

    throughput_col = find_column(
        sample,
        [
            "throughput_eps",
            "throughput"
        ]
    )

    e2e_p50_col = find_column(
        sample,
        [
            "e2e_p50_us",
            "e2e_p50",
            "end_to_end_p50_us"
        ]
    )

    e2e_p95_col = find_column(
        sample,
        [
            "e2e_p95_us",
            "e2e_p95",
            "end_to_end_p95_us"
        ]
    )

    e2e_p99_col = find_column(
        sample,
        [
            "e2e_p99_us",
            "e2e_p99",
            "end_to_end_p99_us"
        ]
    )

    e2e_p999_col = find_column(
        sample,
        [
            "e2e_p999_us",
            "e2e_p99_9_us",
            "e2e_p999",
            "end_to_end_p999_us"
        ]
    )

    e2e_max_col = find_column(
        sample,
        [
            "e2e_max_us",
            "e2e_max",
            "end_to_end_max_us"
        ]
    )

    workers_col = find_column(
        sample,
        [
            "workers",
            "avg_workers",
            "average_workers"
        ]
    )

    queue_p99_col = find_column(
        sample,
        [
            "queue_wait_p99_us",
            "queue_p99_us"
        ]
    )

    throttled_col = find_column(
        sample,
        [
            "throttled",
            "throttled_events"
        ]
    )

    required = [
        rate_col,
        throughput_col,
        e2e_p50_col,
        e2e_p95_col,
        e2e_p99_col,
        e2e_p999_col,
        e2e_max_col
    ]

    if any(x is None for x in required):
        print("\nERROR: Could not identify required CSV columns.")
        print("Detected columns:")
        for c in sample.keys():
            print("  " + c)
        return {}

    groups = {}

    for row in rows:
        try:
            rate = float(row[rate_col])
        except:
            continue

        groups.setdefault(rate, []).append(row)

    result = {}

    for rate, group in sorted(groups.items()):
        def values(col):
            if col is None:
                return []

            output = []

            for r in group:
                try:
                    output.append(float(r[col]))
                except:
                    pass

            return output

        def mean(col):
            x = values(col)
            return statistics.mean(x) if x else float("nan")

        result[rate] = {
            "throughput": mean(throughput_col),
            "p50": mean(e2e_p50_col),
            "p95": mean(e2e_p95_col),
            "p99": mean(e2e_p99_col),
            "p999": mean(e2e_p999_col),
            "max": mean(e2e_max_col),
            "workers": mean(workers_col),
            "queue_p99": mean(queue_p99_col),
            "throttled": mean(throttled_col)
        }

    return result


def pct_change(v1, v2):
    if v1 == 0:
        return float("nan")
    return ((v2 - v1) / v1) * 100.0


def latency_improvement(v1, v2):
    if v1 == 0:
        return float("nan")
    return ((v1 - v2) / v1) * 100.0


print("\n============================================")
print("NEXUSFLOW V1 vs V2 E2E COMPARISON")
print("============================================")

v1_rows = read_csv(v1_path)
v2_rows = read_csv(v2_path)

v1 = aggregate(v1_rows)
v2 = aggregate(v2_rows)

rates = sorted(set(v1.keys()) & set(v2.keys()))

if not rates:
    print("\nERROR: No common load levels found.")
    raise SystemExit

print("\nCommon load levels:")
print(", ".join(f"{int(x)} EPS" for x in rates))

print("\n============================================")
print("COMPARATIVE RESULTS")
print("============================================")

header = (
    f"{'Load':>8} "
    f"{'V1 EPS':>12} {'V2 EPS':>12} {'Thr Δ%':>10} "
    f"{'V1 P99':>12} {'V2 P99':>12} {'P99 Δ%':>10} "
    f"{'V1 P50':>12} {'V2 P50':>12}"
)

print(header)
print("-" * len(header))

comparison = []

for rate in rates:
    a = v1[rate]
    b = v2[rate]

    throughput_delta = pct_change(a["throughput"], b["throughput"])

    p99_delta = latency_improvement(a["p99"], b["p99"])

    p50_delta = latency_improvement(a["p50"], b["p50"])

    print(
        f"{int(rate):>8} "
        f"{a['throughput']:>12.2f} "
        f"{b['throughput']:>12.2f} "
        f"{throughput_delta:>9.2f}% "
        f"{a['p99']:>12.2f} "
        f"{b['p99']:>12.2f} "
        f"{p99_delta:>9.2f}% "
        f"{a['p50']:>12.2f} "
        f"{b['p50']:>12.2f}"
    )

    comparison.append({
        "rate": rate,
        "throughput_delta": throughput_delta,
        "p99_improvement": p99_delta,
        "p50_improvement": p50_delta
    })


print("\n============================================")
print("DETAILED LATENCY COMPARISON")
print("============================================")

header2 = (
    f"{'Load':>8} "
    f"{'P95 V1':>12} {'P95 V2':>12} {'Imp %':>10} "
    f"{'P99 V1':>12} {'P99 V2':>12} {'Imp %':>10} "
    f"{'P99.9 V1':>12} {'P99.9 V2':>12} {'Imp %':>10}"
)

print(header2)
print("-" * len(header2))

for rate in rates:
    a = v1[rate]
    b = v2[rate]

    p95_imp = latency_improvement(a["p95"], b["p95"])
    p99_imp = latency_improvement(a["p99"], b["p99"])
    p999_imp = latency_improvement(a["p999"], b["p999"])

    print(
        f"{int(rate):>8} "
        f"{a['p95']:>12.2f} "
        f"{b['p95']:>12.2f} "
        f"{p95_imp:>9.2f}% "
        f"{a['p99']:>12.2f} "
        f"{b['p99']:>12.2f} "
        f"{p99_imp:>9.2f}% "
        f"{a['p999']:>12.2f} "
        f"{b['p999']:>12.2f} "
        f"{p999_imp:>9.2f}%"
    )


print("\n============================================")
print("WORKER / QUEUE BEHAVIOR")
print("============================================")

header3 = (
    f"{'Load':>8} "
    f"{'Workers V1':>14} {'Workers V2':>14} "
    f"{'Queue P99 V1':>16} {'Queue P99 V2':>16}"
)

print(header3)
print("-" * len(header3))

for rate in rates:
    a = v1[rate]
    b = v2[rate]

    v1_queue = a["queue_p99"]
    v2_queue = b["queue_p99"]

    q1 = f"{v1_queue:.2f}" if v1_queue == v1_queue else "N/A"
    q2 = f"{v2_queue:.2f}" if v2_queue == v2_queue else "N/A"

    print(
        f"{int(rate):>8} "
        f"{a['workers']:>14.2f} "
        f"{b['workers']:>14.2f} "
        f"{q1:>16} "
        f"{q2:>16}"
    )


print("\n============================================")
print("OVERALL V2 EFFECT")
print("============================================")

throughput_changes = [
    x["throughput_delta"]
    for x in comparison
]

p99_changes = [
    x["p99_improvement"]
    for x in comparison
]

p50_changes = [
    x["p50_improvement"]
    for x in comparison
]

print(
    f"Mean throughput change: "
    f"{statistics.mean(throughput_changes):.2f}%"
)

print(
    f"Mean P99 latency improvement: "
    f"{statistics.mean(p99_changes):.2f}%"
)

print(
    f"Mean P50 latency improvement: "
    f"{statistics.mean(p50_changes):.2f}%"
)

throughput_wins = sum(
    1 for x in throughput_changes if x > 0
)

p99_wins = sum(
    1 for x in p99_changes if x > 0
)

p50_wins = sum(
    1 for x in p50_changes if x > 0
)

print(
    f"\nThroughput wins: "
    f"{throughput_wins}/{len(rates)}"
)

print(
    f"P99 latency wins: "
    f"{p99_wins}/{len(rates)}"
)

print(
    f"P50 latency wins: "
    f"{p50_wins}/{len(rates)}"
)


print("\n============================================")
print("BEST OPERATING POINTS")
print("============================================")

best_v1_throughput = max(
    rates,
    key=lambda r: v1[r]["throughput"]
)

best_v2_throughput = max(
    rates,
    key=lambda r: v2[r]["throughput"]
)

best_v1_p99 = min(
    rates,
    key=lambda r: v1[r]["p99"]
)

best_v2_p99 = min(
    rates,
    key=lambda r: v2[r]["p99"]
)

print(
    f"V1 peak throughput: "
    f"{int(best_v1_throughput)} EPS -> "
    f"{v1[best_v1_throughput]['throughput']:.2f} EPS"
)

print(
    f"V2 peak throughput: "
    f"{int(best_v2_throughput)} EPS -> "
    f"{v2[best_v2_throughput]['throughput']:.2f} EPS"
)

print(
    f"V1 lowest P99: "
    f"{int(best_v1_p99)} EPS -> "
    f"{v1[best_v1_p99]['p99']:.2f} us"
)

print(
    f"V2 lowest P99: "
    f"{int(best_v2_p99)} EPS -> "
    f"{v2[best_v2_p99]['p99']:.2f} us"
)


print("\n============================================")
print("SCIENTIFIC INTERPRETATION")
print("============================================")

print(
    "V2 should not be declared superior solely from this experiment."
)

print(
    "The correct conclusion must be based on paired run-level "
    "statistics, effect sizes, confidence intervals, and significance tests."
)

print(
    "Large E2E latency with low processing latency indicates that "
    "queueing/admission/scheduling dominates the measured E2E path."
)

print(
    "Worker scaling behavior must therefore be evaluated together "
    "with queue wait, P99/P99.9 latency, throughput, and throttling."
)

print("\n============================================")
print("COMPARISON COMPLETE")
print("============================================")
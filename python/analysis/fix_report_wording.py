from pathlib import Path

path = Path(
    "benchmarks/results/repeated/experiment_report.md"
)

text = path.read_text(
    encoding="utf-8"
)

text = text.replace(
    "wasnot",
    "was not"
)

text = text.replace(
    "a singlescheduling strategy",
    "a single scheduling strategy"
)

path.write_text(
    text,
    encoding="utf-8"
)

print("Final report wording corrected.")

from pathlib import Path
import csv
import sys


PROJECT_ROOT = Path(__file__).resolve().parents[2]

RAW_DIR = PROJECT_ROOT / "data" / "raw"
PROCESSED_DIR = PROJECT_ROOT / "data" / "processed"

INPUT_FILE = RAW_DIR / "transactions.csv"
OUTPUT_FILE = PROCESSED_DIR / "nexusflow_events.csv"


def classify_priority(amount: float) -> str:
    if amount >= 100000:
        return "CRITICAL"

    if amount >= 50000:
        return "HIGH"

    if amount >= 10000:
        return "NORMAL"

    return "LOW"


def prepare_dataset() -> None:
    if not INPUT_FILE.exists():
        print(
            "ERROR: Dataset not found."
        )

        print(
            f"Expected file: {INPUT_FILE}"
        )

        print(
            "Place the downloaded dataset in data/raw/transactions.csv."
        )

        return

    PROCESSED_DIR.mkdir(
        parents=True,
        exist_ok=True
    )

    processed_rows = 0

    with INPUT_FILE.open(
        "r",
        encoding="utf-8-sig",
        newline=""
    ) as source:

        reader = csv.DictReader(source)

        if reader.fieldnames is None:
            print(
                "ERROR: Dataset does not contain a header."
            )
            return

        field_map = {
            name.strip().lower(): name
            for name in reader.fieldnames
        }

        timestamp_column = None
        amount_column = None

        timestamp_candidates = [
            "timestamp",
            "time",
            "datetime",
            "date"
        ]

        amount_candidates = [
            "amount",
            "transaction_amount",
            "value"
        ]

        for candidate in timestamp_candidates:
            if candidate in field_map:
                timestamp_column = field_map[candidate]
                break

        for candidate in amount_candidates:
            if candidate in field_map:
                amount_column = field_map[candidate]
                break

        if timestamp_column is None:
            print(
                "ERROR: No timestamp column found."
            )
            return

        if amount_column is None:
            print(
                "ERROR: No amount column found."
            )
            return

        with OUTPUT_FILE.open(
            "w",
            encoding="utf-8",
            newline=""
        ) as destination:

            writer = csv.writer(destination)

            writer.writerow([
                "timestamp_us",
                "value",
                "priority"
            ])

            for row in reader:

                timestamp_text = (
                    row.get(timestamp_column, "")
                    .strip()
                )

                amount_text = (
                    row.get(amount_column, "")
                    .strip()
                )

                if not timestamp_text:
                    continue

                if not amount_text:
                    continue

                try:
                    amount = float(amount_text)
                except ValueError:
                    continue

                timestamp_us = processed_rows * 1000

                priority = classify_priority(
                    amount
                )

                writer.writerow([
                    timestamp_us,
                    amount,
                    priority
                ])

                processed_rows += 1

    print(
        "============================================="
    )

    print(
        "NexusFlow Dataset Preparation"
    )

    print(
        "============================================="
    )

    print(
        f"Input dataset: {INPUT_FILE}"
    )

    print(
        f"Output dataset: {OUTPUT_FILE}"
    )

    print(
        f"Processed events: {processed_rows}"
    )

    print(
        "Dataset preparation completed."
    )


if __name__ == "__main__":
    prepare_dataset()

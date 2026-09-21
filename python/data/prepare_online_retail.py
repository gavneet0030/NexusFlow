from pathlib import Path
import csv
import math

import pandas as pd


PROJECT_ROOT = Path(__file__).resolve().parents[2]

INPUT_FILE = (
    PROJECT_ROOT
    / "data"
    / "raw"
    / "online_retail_II.xlsx"
)

OUTPUT_FILE = (
    PROJECT_ROOT
    / "data"
    / "processed"
    / "nexusflow_events.csv"
)


def classify_priority(
    transaction_value: float,
    quantity: float
) -> str:

    if transaction_value >= 1000 or quantity >= 100:
        return "CRITICAL"

    if transaction_value >= 500 or quantity >= 50:
        return "HIGH"

    if transaction_value >= 100:
        return "NORMAL"

    return "LOW"


def timestamp_to_microseconds(
    timestamp: pd.Timestamp
) -> int:

    return int(
        timestamp.timestamp() * 1_000_000
    )


def main() -> None:

    print("=============================================")
    print("NexusFlow - Online Retail II Preprocessing")
    print("=============================================")
    print()

    if not INPUT_FILE.exists():
        print("ERROR: Input dataset was not found.")
        print(f"Expected file: {INPUT_FILE}")
        return

    print("Loading Excel dataset...")
    print(f"Input: {INPUT_FILE}")
    print()

    sheets = pd.ExcelFile(
        INPUT_FILE,
        engine="openpyxl"
    ).sheet_names

    print("Available sheets:")

    for sheet in sheets:
        print(f"  - {sheet}")

    print()

    frames = []

    for sheet in sheets:

        print(f"Reading sheet: {sheet}")

        frame = pd.read_excel(
            INPUT_FILE,
            sheet_name=sheet,
            engine="openpyxl"
        )

        frame.columns = [
            str(column).strip()
            for column in frame.columns
        ]

        frames.append(frame)

        print(
            f"Rows loaded from {sheet}: "
            f"{len(frame)}"
        )

    if not frames:
        print("ERROR: No sheets were loaded.")
        return

    data = pd.concat(
        frames,
        ignore_index=True
    )

    print()
    print(
        f"Total raw rows: {len(data)}"
    )

    required_columns = [
        "Invoice",
        "StockCode",
        "Description",
        "Quantity",
        "InvoiceDate",
        "Price",
        "Customer ID",
        "Country"
    ]

    missing_columns = [
        column
        for column in required_columns
        if column not in data.columns
    ]

    if missing_columns:

        print()
        print("ERROR: Required columns are missing:")

        for column in missing_columns:
            print(f"  - {column}")

        print()
        print("Actual columns found:")

        for column in data.columns:
            print(f"  - {column}")

        return

    # -----------------------------------------
    # Basic cleaning
    # -----------------------------------------

    data = data.dropna(
        subset=[
            "InvoiceDate",
            "Quantity",
            "Price"
        ]
    ).copy()

    data["InvoiceDate"] = pd.to_datetime(
        data["InvoiceDate"],
        errors="coerce"
    )

    data["Quantity"] = pd.to_numeric(
        data["Quantity"],
        errors="coerce"
    )

    data["Price"] = pd.to_numeric(
        data["Price"],
        errors="coerce"
    )

    data = data.dropna(
        subset=[
            "InvoiceDate",
            "Quantity",
            "Price"
        ]
    )

    # Remove invalid transaction values.
    data = data[
        (data["Quantity"] > 0)
        & (data["Price"] > 0)
    ].copy()

    # -----------------------------------------
    # Sort chronologically
    # -----------------------------------------

    data = data.sort_values(
        "InvoiceDate"
    ).reset_index(
        drop=True
    )

    # -----------------------------------------
    # Create NexusFlow event dataset
    # -----------------------------------------

    events = []

    for index, row in data.iterrows():

        quantity = float(
            row["Quantity"]
        )

        unit_price = float(
            row["Price"]
        )

        transaction_value = (
            quantity * unit_price
        )

        timestamp_us = (
            timestamp_to_microseconds(
                row["InvoiceDate"]
            )
        )

        priority = classify_priority(
            transaction_value,
            quantity
        )

        events.append({
            "event_id": int(index),
            "timestamp_us": timestamp_us,
            "value": transaction_value,
            "priority": priority,
            "source": "online_retail_II",
            "type": "retail_transaction",
            "quantity": quantity,
            "unit_price": unit_price,
            "stock_code": str(
                row["StockCode"]
            ),
            "customer_id": (
                str(row["Customer ID"])
                if not pd.isna(row["Customer ID"])
                else ""
            ),
            "country": str(
                row["Country"]
            )
        })

    output = pd.DataFrame(events)

    # -----------------------------------------
    # Remove invalid numeric values
    # -----------------------------------------

    output = output[
        output["value"].apply(
            lambda value:
                math.isfinite(float(value))
        )
    ].copy()

    # -----------------------------------------
    # Save processed dataset
    # -----------------------------------------

    output.to_csv(
        OUTPUT_FILE,
        index=False,
        encoding="utf-8",
        quoting=csv.QUOTE_MINIMAL
    )

    # -----------------------------------------
    # Summary
    # -----------------------------------------

    print()
    print("=============================================")
    print("PREPROCESSING COMPLETED")
    print("=============================================")

    print(
        f"Raw rows: {len(data)}"
    )

    print(
        f"Processed events: {len(output)}"
    )

    print(
        f"Output: {OUTPUT_FILE}"
    )

    print()
    print("Priority distribution:")

    priority_counts = (
        output["priority"]
        .value_counts()
    )

    for priority, count in (
        priority_counts.items()
    ):
        print(
            f"  {priority}: {count}"
        )

    print()
    print("Event dataset columns:")

    for column in output.columns:
        print(f"  - {column}")

    print()
    print("=============================================")


if __name__ == "__main__":
    main()

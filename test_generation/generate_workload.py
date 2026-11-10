"""Generate headerless rows: search|insert|delete followed by space-separated floats."""

import argparse
from decimal import Decimal, InvalidOperation
from pathlib import Path
import random


OPERATIONS = ("search", "insert", "delete")


def proportion(text: str) -> Decimal:
    try:
        value = Decimal(text)
    except InvalidOperation as error:
        raise argparse.ArgumentTypeError("must be a nonnegative number") from error
    if not value.is_finite() or value < 0:
        raise argparse.ArgumentTypeError("must be a finite nonnegative number")
    return value


def parse_arguments(argv=None):
    parser = argparse.ArgumentParser(
        description="Write a random mixed workload to testing_files. Each row is an operation and a vector."
    )
    parser.add_argument("q", type=int, help="Total number of operations (positive).")
    parser.add_argument("n", type=int, help="Coordinates per vector (positive; use 128 for SIFT).")
    parser.add_argument("filename", help="Output filename inside testing_files.")
    parser.add_argument("--queries", type=proportion, required=True, help="Fraction of search operations.")
    parser.add_argument("--inserts", type=proportion, required=True, help="Fraction of insert operations.")
    parser.add_argument("--deletes", type=proportion, required=True, help="Fraction of delete operations.")
    args = parser.parse_args(argv)
    if args.q <= 0 or args.n <= 0:
        parser.error("q and n must be positive")
    name = Path(args.filename)
    if not args.filename or name.name != args.filename or name in (Path("."), Path("..")):
        parser.error("filename must be a name without a directory")
    if args.queries + args.inserts + args.deletes != Decimal(1):
        parser.error("--queries, --inserts, and --deletes must sum to 1")
    return args


def operation_counts(total, proportions):
    """Round fractional counts to integers while preserving the total count."""
    exact = [total * value for value in proportions]
    counts = [int(value) for value in exact]
    order = sorted(range(3), key=lambda i: exact[i] - counts[i], reverse=True)
    for i in order[:total - sum(counts)]:
        counts[i] += 1
    return counts


def write_workload(output_path, total, n, proportions):
    counts = operation_counts(total, proportions)
    remaining = counts.copy()
    rng = random.Random()
    with output_path.open("w", encoding="utf-8", newline="\n") as output:
        for _ in range(total):
            operation = rng.choices(range(3), weights=remaining, k=1)[0]
            remaining[operation] -= 1
            vector = " ".join(f"{rng.uniform(0.0, 255.0):.6f}" for _ in range(n))
            output.write(f"{OPERATIONS[operation]} {vector}\n")
    return counts


def main(argv=None):
    args = parse_arguments(argv)
    output_dir = Path(__file__).resolve().parent.parent / "testing_files"
    output_dir.mkdir(parents=True, exist_ok=True)
    output_path = output_dir / args.filename
    counts = write_workload(output_path, args.q, args.n, (args.queries, args.inserts, args.deletes))
    print(f"Wrote {args.q} operations of dimension {args.n} to {output_path}")
    print(f"search: {counts[0]}, insert: {counts[1]}, delete: {counts[2]}")


if __name__ == "__main__":
    main()

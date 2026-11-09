import argparse
from pathlib import Path
import random


parser = argparse.ArgumentParser(
    description="Write random query vectors to the repository's testing_files folder."
)
parser.add_argument("q", type=int, help="Number of queries (zero or greater).")
parser.add_argument("n", type=int, help="Coordinates per query (positive; use 128 for SIFT).")
parser.add_argument("filename", help="Output filename inside testing_files, e.g. queries.txt.")
args = parser.parse_args()
if args.q <= 0:
    parser.error("q must be positive")
if args.n <= 0:
    parser.error("n must be positive")
name = Path(args.filename)
if not args.filename or name.name != args.filename or name in (Path("."), Path("..")):
    parser.error("filename must be a name without a directory")

output_dir = Path(__file__).resolve().parent.parent / "testing_files"
output_dir.mkdir(parents=True, exist_ok=True)
output_path = output_dir / name
rng = random.Random()

with output_path.open("w", encoding="utf-8", newline="\n") as output:
    output.write(f"{args.q}\n{args.n}\n")
    for i in range(args.q):
        output.write(" ".join(f"{rng.uniform(0.0, 255.0):.6f}" for _ in range(args.n)))
        output.write("\n")
print(f"Wrote {args.q} queries of dimension {args.n} to {output_path}")

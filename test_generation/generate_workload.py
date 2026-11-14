"""Generate headerless rows: search|insert|delete followed by space-separated floats."""

import argparse
import bisect
from decimal import Decimal, InvalidOperation
import math
from pathlib import Path
import random
import struct

OPERATIONS = ("search", "insert", "delete")
REPOSITORY = Path(__file__).resolve().parent.parent
SIFT_DIMENSION = 128
POINT_COUNT = 1_000_000
CENTROID_COUNT = 256

# Parses decimals strings from the command line to get the portion of requests for each query type.
def proportion(text: str) -> Decimal:
    value = Decimal(text)
    if value < 0:
        raise argparse.ArgumentTypeError("must be a nonnegative number")
    return value

# Parses float strings from the command line to get the zipfian parameter.
def zipf_skew(text: str) -> float:
    value = float(text)
    if not value < 0:
        raise argparse.ArgumentTypeError("must be a nonnegative number")
    return value

# Parse the command line arguments
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
    parser.add_argument(
        "--zipf-skew", type=zipf_skew, default=None, metavar="S",
        help="Target centroid rank r with probability proportional to r^-S. "
             "0 selects clusters uniformly; larger values increase skew. "
             "Without this flag, coordinates remain uniform random floats.",
    )
    parser.add_argument(
        "--index", type=Path, default=REPOSITORY / "sift" / "sift_l2.packed.bin",
        help="Packed SIFT index providing centroids for --zipf-skew.",
    )
    parser.add_argument("--seed", type=int, default=None, help="Optional reproducible random seed.")
    args = parser.parse_args(argv)
    if args.q <= 0 or args.n <= 0:
        parser.error("q and n must be positive")
    name = Path(args.filename)
    if not args.filename or name.name != args.filename or name in (Path("."), Path("..")):
        parser.error("filename must be a name without a directory")
    if args.queries + args.inserts + args.deletes != Decimal(1):
        parser.error("--queries, --inserts, and --deletes must sum to 1")
    if args.zipf_skew is not None and args.n != SIFT_DIMENSION:
        parser.error(f"--zipf-skew requires n={SIFT_DIMENSION} for the packed SIFT index")
    return args

# Gets the quantity of each operation for the total benchmarking run.
def operation_counts(total, proportions):
    return [int(total * value) for value in proportions]

# Reads the centroids from the file and saves it as a list
def load_centroids(index_path):
    points_bytes = POINT_COUNT * SIFT_DIMENSION * 4
    centroid_bytes = CENTROID_COUNT * SIFT_DIMENSION * 4
    expected_bytes = points_bytes + centroid_bytes + CENTROID_COUNT * 8
    with index_path.open("rb") as source:
        source.seek(0, 2)
        source.seek(points_bytes)
        data = source.read(centroid_bytes)
    centroids = list(struct.iter_unpack(f"<{SIFT_DIMENSION}f", data))
    return centroids

# Handles the Zipfian parameter skew balancing
class ZipfVectors:
    def __init__(self, centroids, skew):
        self.centroids = centroids
        self.cumulative = []
        total = 0.0

        # Sets the sampling distribution to the zipfian distribution with the skew set.
        # The zipfian skew follows an exponential decay system.
        for rank in range(1, len(centroids) + 1):
            total += math.exp(-skew * math.log(rank))
            self.cumulative.append(total)
        self.jitter = []

        # This calculates a safe level of variation to still be in the same cluster
        # TODO: Maybe don't ensure that this is strictly inside this. Something like a
        # normal distribution around the centroid point would be more representative.
        for i, centroid in enumerate(centroids):
            separation = min(
                math.sqrt(sum((a - b) ** 2 for a, b in zip(centroid, other)))
                for j, other in enumerate(centroids) if j != i
            )
            self.jitter.append(separation / (4 * math.sqrt(len(centroid))))

    # Sample based on the probabilities set above
    def sample(self, rng):
        cluster = bisect.bisect_right(self.cumulative, rng.random() * self.cumulative[-1])
        cluster = min(cluster, len(self.centroids) - 1)
        centroid = self.centroids[cluster]
        jitter = self.jitter[cluster]
        return " ".join(f"{x + rng.uniform(-jitter, jitter):.9g}" for x in centroid)

# Writes the workload to the output file. This samples from the ZipfianVectors class is avaialable, and is random otherwise
def write_workload(output_path, total, n, proportions, vectors=None, seed=None):
    counts = operation_counts(total, proportions)
    remaining = counts.copy()
    rng = random.Random(seed)
    with output_path.open("w", encoding="utf-8", newline="\n") as output:
        for _ in range(total):
            operation = rng.choices(range(3), weights=remaining, k=1)[0]
            remaining[operation] -= 1
            vector = (vectors.sample(rng) if vectors is not None else " ".join(f"{rng.uniform(0.0, 255.0):.6f}" for _ in range(n)))
            output.write(f"{OPERATIONS[operation]} {vector}\n")
    return counts

# Runs the main workload generation
def main(argv=None):
    args = parse_arguments(argv)
    vectors = None
    if args.zipf_skew is not None:
        vectors = ZipfVectors(load_centroids(args.index), args.zipf_skew)
    output_dir = REPOSITORY / "testing_files"
    output_dir.mkdir(parents=True, exist_ok=True)
    output_path = output_dir / args.filename
    counts = write_workload(output_path, args.q, args.n, (args.queries, args.inserts, args.deletes), vectors=vectors, seed=args.seed)
    print(f"Wrote {args.q} operations of dimension {args.n} to {output_path}")
    print(f"search: {counts[0]}, insert: {counts[1]}, delete: {counts[2]}")


if __name__ == "__main__":
    main()

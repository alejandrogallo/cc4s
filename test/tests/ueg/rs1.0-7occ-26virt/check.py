#!/usr/bin/env python3

from testis import read_yaml, compare_energies

out = read_yaml("cc4s.out")

assert out["dryRun"] == 0, "We should not be doing dryRuns now"

compare_energies("correct.out.yaml", "cc4s.out.yaml", accuracy=1e-7)

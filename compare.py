#!/usr/bin/env python3
"""Compare Sobel outputs against the original reference image.
Usage: python3 compare.py out_original.png out_optimized_1t.png [more.png ...]
"""
import sys
import cv2
import numpy as np

if len(sys.argv) < 3:
    sys.exit("Usage: python3 compare.py <reference.png> <test.png> [test.png ...]")

ref = cv2.imread(sys.argv[1], cv2.IMREAD_UNCHANGED)
if ref is None:
    sys.exit(f"Could not read {sys.argv[1]}")
ref = ref.astype(np.int32)
print(f"reference: {sys.argv[1]}  shape={ref.shape}")

all_pass = True
for path in sys.argv[2:]:
    img = cv2.imread(path, cv2.IMREAD_UNCHANGED)
    if img is None:
        print(f"{path}: could not read file  FAIL")
        all_pass = False
        continue
    if img.shape != ref.shape:
        print(f"{path}: shape {img.shape} != reference {ref.shape}  FAIL")
        all_pass = False
        continue

    diff = np.abs(ref - img.astype(np.int32))
    max_diff = int(diff.max())
    differing = int(np.count_nonzero(diff))
    ok = (max_diff == 0)
    all_pass &= ok

    line = f"{path}: max_diff={max_diff} differing_pixels={differing}  {'PASS' if ok else 'FAIL'}"
    if not ok:
        ys, xs = np.nonzero(diff)
        line += f"  first_diff_at(y={ys[0]}, x={xs[0]}) rows_affected={len(np.unique(ys))}"
    print(line)

sys.exit(0 if all_pass else 1)

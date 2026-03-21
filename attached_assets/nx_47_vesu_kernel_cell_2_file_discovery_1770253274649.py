# CELL 2 — FILE DISCOVERY (NX47 V10.3)
# Responsibility: recursively discover all files, no assumptions

import os
from collections import defaultdict

logger.log("CELL2_START_FILE_DISCOVERY")

all_files = []
for root, dirs, files in os.walk(dataset_path):
    for f in files:
        all_files.append(os.path.join(root, f))

logger.log(
    "CELL2_DISCOVERY_COMPLETE",
    data={"total_files": len(all_files), "sample": all_files[:25]}
)

if not all_files:
    raise RuntimeError("CELL2_FAILURE: dataset root contains no files")

print(f"CELL 2 OK — {len(all_files)} FILES DISCOVERED")

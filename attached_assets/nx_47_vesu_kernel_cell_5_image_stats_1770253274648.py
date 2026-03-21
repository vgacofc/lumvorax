# CELL 5 — IMAGE STATISTICS (NX47 V10.3)
# Responsibility: compute real stats and checksums

import numpy as np
import hashlib

logger.log("CELL5_START_IMAGE_STATS")

stats = []
for path, size, mode in loaded:
    with open(path, "rb") as f:
        sha = hashlib.sha256(f.read()).hexdigest()
    img = Image.open(path)
    arr = np.array(img)
    stats.append({
        "file": os.path.basename(path),
        "shape": arr.shape,
        "dtype": str(arr.dtype),
        "min": int(arr.min()),
        "max": int(arr.max()),
        "mean": float(arr.mean()),
        "sha256": sha
    })

logger.log("CELL5_IMAGE_STATS", data={"images": stats})

print("CELL 5 OK — IMAGE STATS COMPUTED")

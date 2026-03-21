# CELL 1 — ENVIRONMENT & DATASET AUDIT (NX47 V10.3)
# Responsibility: verify Kaggle environment and locate dataset root

import os
import time
import json
import hashlib
import threading
from typing import Optional, Dict

# --------------------------------------------------
# Forensic Logger (shared pattern)
# --------------------------------------------------
class ForensicLogger:
    def __init__(self, outdir="/kaggle/working/nx47_logs"):
        os.makedirs(outdir, exist_ok=True)
        self.sid = hashlib.sha256(str(time.time_ns()).encode()).hexdigest()[:16]
        self.path = os.path.join(outdir, f"nx47_{self.sid}.log")
        self.lock = threading.Lock()

    def log(self, msg: str, level: str = "INFO", data: Optional[Dict] = None):
        ts = time.time_ns()
        line = f"[{ts}][{level}] {msg}"
        if data is not None:
            line += " | DATA: " + json.dumps(data, ensure_ascii=False)
        print(line)
        with self.lock:
            with open(self.path, "a", encoding="utf-8") as f:
                f.write(line + "\n")

logger = ForensicLogger()

# --------------------------------------------------
# Dataset audit
# --------------------------------------------------
EXPECTED_DATASET = "vesuvius-challenge-surface-detection"
BASE_PATH = "/kaggle/input"

logger.log("CELL1_START_ENV_AUDIT", data={"base_path": BASE_PATH})

if not os.path.exists(BASE_PATH):
    logger.log("NON_KAGGLE_ENVIRONMENT", level="ERROR")
    raise RuntimeError("/kaggle/input not found")

entries = os.listdir(BASE_PATH)
logger.log("INPUT_DIRECTORY_LISTED", data={"entries": entries})

if EXPECTED_DATASET not in entries:
    logger.log("DATASET_NOT_FOUND", level="ERROR", data={"expected": EXPECTED_DATASET})
    raise RuntimeError("Vesuvius dataset not attached")

dataset_path = os.path.join(BASE_PATH, EXPECTED_DATASET)
logger.log("DATASET_ROOT_CONFIRMED", data={"dataset_path": dataset_path})

print("CELL 1 OK — DATASET ROOT VERIFIED")

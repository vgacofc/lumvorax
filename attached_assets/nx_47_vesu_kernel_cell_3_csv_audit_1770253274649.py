# CELL 3 — CSV AUDIT (NX47 V10.3)
# Responsibility: validate train.csv structure

import pandas as pd

logger.log("CELL3_START_CSV_AUDIT")

csv_path = os.path.join(dataset_path, "train.csv")
if not os.path.exists(csv_path):
    raise RuntimeError("CELL3_FAILURE: train.csv not found")

train_df = pd.read_csv(csv_path)

logger.log(
    "CELL3_CSV_LOADED",
    data={
        "rows": len(train_df),
        "columns": list(train_df.columns)
    }
)

print("CELL 3 OK — train.csv LOADED")

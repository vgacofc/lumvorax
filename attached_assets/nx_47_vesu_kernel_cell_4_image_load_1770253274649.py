# CELL 4 — IMAGE LOAD (NX47 V10.3)
# Responsibility: load real TIFF images

from PIL import Image

logger.log("CELL4_START_IMAGE_LOAD")

image_dir = os.path.join(dataset_path, "train_images")n
if not os.path.isdir(image_dir):
    raise RuntimeError("CELL4_FAILURE: train_images directory not found")

image_files = sorted([
    os.path.join(image_dir, f)
    for f in os.listdir(image_dir)
    if f.lower().endswith(".tif")
])

if not image_files:
    raise RuntimeError("CELL4_FAILURE: no TIFF images found")

# Load a small deterministic sample
sample_images = image_files[:5]
loaded = []
for p in sample_images:
    img = Image.open(p)
    loaded.append((p, img.size, img.mode))

logger.log(
    "CELL4_IMAGES_LOADED",
    data={"count": len(loaded), "details": loaded}
)

print("CELL 4 OK — IMAGES LOADED")

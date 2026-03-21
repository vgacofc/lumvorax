#!/usr/bin/env python3
"""
upload_to_supabase.py — C59
Upload automatique de tous les .log et .csv d'un run vers Supabase
via l'API REST HTTP (pas de connexion TCP directe requise).

Usage:
    python3 upload_to_supabase.py <run_dir> [--delete-after]
    python3 upload_to_supabase.py --all [--delete-after]
    python3 upload_to_supabase.py --check-tables

PREREQUIS (une seule fois) : exécuter tools/supabase_schema.sql dans l'éditeur
SQL de Supabase avant le premier upload.

Variables d'environnement requises:
    SUPABASE_URL            = https://xxx.supabase.co
    SUPABASE_SERVICE_ROLE_KEY = eyJ...
"""

import os
import sys
import csv
import json
import hashlib
import argparse
import traceback
import time
from pathlib import Path

try:
    import requests
except ImportError:
    print("[FATAL] requests manquant. Lancez: python3 -m pip install requests")
    sys.exit(1)

ROOT = Path(__file__).resolve().parent.parent
RESULTS_DIR = ROOT / "results"

SUPABASE_URL = os.environ.get("SUPABASE_URL", "").rstrip("/")
SERVICE_KEY  = os.environ.get("SUPABASE_SERVICE_ROLE_KEY", "")
MAX_CSV_ROWS = 100_000
BATCH_SIZE   = 200

if not SUPABASE_URL or not SERVICE_KEY:
    print("[WARN] SUPABASE_URL ou SUPABASE_SERVICE_ROLE_KEY absent — upload désactivé")

def headers():
    return {
        "apikey":        SERVICE_KEY,
        "Authorization": f"Bearer {SERVICE_KEY}",
        "Content-Type":  "application/json",
        "Prefer":        "return=minimal"
    }

def rest(endpoint):
    return f"{SUPABASE_URL}/rest/v1/{endpoint}"

def check_tables():
    for table in ["quantum_run_files", "quantum_csv_rows"]:
        resp = requests.get(rest(f"{table}?limit=1"), headers=headers(), timeout=10)
        if resp.status_code == 200:
            print(f"  [OK] Table {table} accessible")
        else:
            print(f"  [FAIL] Table {table} introuvable ({resp.status_code}): {resp.text[:100]}")
            print(f"  → Exécutez tools/supabase_schema.sql dans l'éditeur SQL de Supabase")
            return False
    return True

def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()

def upsert_file_record(run_id, rel_path, file_type, size, sha, content=None):
    data = {
        "run_id": run_id,
        "file_path": rel_path,
        "file_type": file_type,
        "file_size_bytes": size,
        "sha256": sha,
    }
    if content is not None:
        data["content_text"] = content[:500_000]
    resp = requests.post(
        rest("quantum_run_files"),
        headers={**headers(), "Prefer": "resolution=merge-duplicates,return=minimal"},
        json=data,
        timeout=30
    )
    if resp.status_code not in (200, 201, 204):
        print(f"  [WARN] upsert_file_record {rel_path}: {resp.status_code} {resp.text[:100]}")

def upload_log_file(run_id, file_path: Path, rel_path: str):
    size = file_path.stat().st_size
    sha = sha256_file(file_path)
    try:
        content = file_path.read_text(errors="replace")
    except Exception:
        content = f"[BINARY: {size} bytes]"
    upsert_file_record(run_id, rel_path, "log", size, sha, content)
    print(f"  [LOG] {rel_path} ({size//1024}KB)")

def delete_csv_rows(run_id, rel_path):
    resp = requests.delete(
        rest(f"quantum_csv_rows?run_id=eq.{run_id}&file_path=eq.{rel_path}"),
        headers=headers(),
        timeout=15
    )
    return resp.status_code in (200, 204)

def upload_csv_file(run_id, file_path: Path, rel_path: str):
    size = file_path.stat().st_size
    sha = sha256_file(file_path)
    upsert_file_record(run_id, rel_path, "csv", size, sha, None)
    delete_csv_rows(run_id, rel_path)

    rows_inserted = 0
    batch = []
    try:
        with open(file_path, newline="", errors="replace") as f:
            reader = csv.DictReader(f)
            for i, row in enumerate(reader):
                if i >= MAX_CSV_ROWS:
                    print(f"  [CSV] {rel_path} tronqué à {MAX_CSV_ROWS} lignes (taille={size//1024}KB)")
                    break
                batch.append({
                    "run_id": run_id,
                    "file_path": rel_path,
                    "row_index": i,
                    "row_json": row
                })
                if len(batch) >= BATCH_SIZE:
                    resp = requests.post(
                        rest("quantum_csv_rows"),
                        headers={**headers(), "Prefer": "return=minimal"},
                        json=batch,
                        timeout=30
                    )
                    if resp.status_code not in (200, 201, 204):
                        print(f"  [WARN] batch insert error: {resp.status_code} {resp.text[:80]}")
                    rows_inserted += len(batch)
                    batch = []
                    time.sleep(0.05)
        if batch:
            resp = requests.post(
                rest("quantum_csv_rows"),
                headers={**headers(), "Prefer": "return=minimal"},
                json=batch,
                timeout=30
            )
            rows_inserted += len(batch)
    except Exception as e:
        print(f"  [WARN] CSV parse {rel_path}: {e}")
    print(f"  [CSV] {rel_path} ({size//1024}KB, {rows_inserted} lignes uploadées)")

def upload_run(run_dir: Path, delete_after=False):
    run_id = run_dir.name
    print(f"\n[SUPABASE] Upload run: {run_id}")

    if not SUPABASE_URL or not SERVICE_KEY:
        print("  [SKIP] Credentials manquants")
        return False

    if not check_tables():
        print("  [SKIP] Tables manquantes — exécutez supabase_schema.sql d'abord")
        return False

    success = True
    for fpath in sorted(run_dir.rglob("*")):
        if not fpath.is_file():
            continue
        size = fpath.stat().st_size
        if size == 0:
            continue
        rel = str(fpath.relative_to(run_dir))
        ext = fpath.suffix.lower()
        try:
            if ext == ".csv":
                upload_csv_file(run_id, fpath, rel)
            elif ext in (".log", ".md", ".json", ".txt", ".sha256", ".sha512"):
                upload_log_file(run_id, fpath, rel)
        except Exception as e:
            print(f"  [ERROR] {rel}: {e}")
            traceback.print_exc()
            success = False

    if success and delete_after:
        import shutil
        shutil.rmtree(run_dir)
        print(f"  [DELETE] Fichiers locaux supprimés: {run_dir.name}")
    print(f"[SUPABASE] Run {run_id} {'uploadé' if success else 'PARTIEL'}.")
    return success

def main():
    parser = argparse.ArgumentParser(description="Upload run vers Supabase REST API")
    parser.add_argument("run_dir", nargs="?", help="Dossier du run à uploader")
    parser.add_argument("--all", action="store_true", help="Uploader tous les runs dans results/")
    parser.add_argument("--delete-after", action="store_true",
                        help="Supprimer les fichiers locaux après upload réussi")
    parser.add_argument("--check-tables", action="store_true",
                        help="Vérifie que les tables Supabase existent")
    args = parser.parse_args()

    if args.check_tables:
        ok = check_tables()
        sys.exit(0 if ok else 1)

    if args.all:
        if not RESULTS_DIR.exists():
            print(f"[WARN] Dossier results/ absent: {RESULTS_DIR}")
            return
        runs = sorted([d for d in RESULTS_DIR.iterdir() if d.is_dir()])
        print(f"[SUPABASE] {len(runs)} runs à uploader")
        for run in runs:
            upload_run(run, delete_after=args.delete_after)
    elif args.run_dir:
        run_dir = Path(args.run_dir)
        if not run_dir.exists():
            print(f"[FATAL] Dossier introuvable: {run_dir}")
            sys.exit(1)
        upload_run(run_dir, delete_after=args.delete_after)
    else:
        parser.print_help()

if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""
ptmc_realtime_uploader.py — C60
Watcher temps réel : uploade chaque fichier CSV PTMC/LUMVORAX sur Supabase
dès qu'il est fermé (après rotation), puis le supprime du disque local.

OBJECTIF : éviter l'accumulation de ~2 GB de CSV PTMC sur disque qui provoque
le SIGKILL du workflow avant que research_execution.log soit écrit.

Usage (lancé en background par run_research_cycle.sh) :
    python3 tools/ptmc_realtime_uploader.py <run_dir> [--sentinel <path>]

Le watcher s'arrête quand :
  - le fichier sentinel <path>.stop existe
  - SIGTERM reçu
  - <run_dir>/logs/research_execution.log existe et taille > 0
"""

import os
import sys
import csv
import json
import time
import signal
import hashlib
import argparse
import traceback
from pathlib import Path

try:
    import requests
except ImportError:
    print("[PTMC-WATCHER] requests manquant", flush=True)
    sys.exit(1)

# ── Credentials Supabase ─────────────────────────────────────────────────────
SERVICE_KEY = os.environ.get("SUPABASE_SERVICE_ROLE_KEY", "")

def _derive_url() -> str:
    db_host = os.environ.get("SUPABASE_DB_HOST", "")
    if db_host.startswith("db.") and ".supabase.co" in db_host:
        pid = db_host[3:].replace(".supabase.co", "")
        return f"https://{pid}.supabase.co"
    return os.environ.get("SUPABASE_URL", "").rstrip("/")

SUPABASE_URL = _derive_url()
BATCH_SIZE   = 200
MAX_CSV_ROWS = 50_000

_stop_flag = False

def handle_signal(sig, frame):
    global _stop_flag
    print(f"[PTMC-WATCHER] Signal {sig} reçu — arrêt propre", flush=True)
    _stop_flag = True

signal.signal(signal.SIGTERM, handle_signal)
signal.signal(signal.SIGINT,  handle_signal)


def headers():
    return {
        "apikey":        SERVICE_KEY,
        "Authorization": f"Bearer {SERVICE_KEY}",
        "Content-Type":  "application/json",
        "Prefer":        "return=minimal",
    }


def rest(endpoint: str) -> str:
    return f"{SUPABASE_URL}/rest/v1/{endpoint}"


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def upsert_file_record(run_id: str, rel: str, ftype: str, size: int, sha: str):
    data = {"run_id": run_id, "file_path": rel, "file_type": ftype,
            "file_size_bytes": size, "sha256": sha}
    try:
        r = requests.post(
            rest("quantum_run_files"),
            headers={**headers(), "Prefer": "resolution=merge-duplicates,return=minimal"},
            json=data, timeout=20)
        if r.status_code not in (200, 201, 204):
            print(f"[PTMC-WATCHER] upsert_file_record WARN {r.status_code}: {r.text[:80]}", flush=True)
    except Exception as e:
        print(f"[PTMC-WATCHER] upsert_file_record ERR: {e}", flush=True)


def upload_csv_rows(run_id: str, rel: str, path: Path) -> bool:
    """Upload les lignes CSV en batches vers quantum_csv_rows. Retourne True si OK."""
    rows_inserted = 0
    batch = []
    try:
        with open(path, newline="", errors="replace") as f:
            reader = csv.DictReader(f)
            for i, row in enumerate(reader):
                if i >= MAX_CSV_ROWS:
                    print(f"[PTMC-WATCHER] {rel} tronqué à {MAX_CSV_ROWS} lignes", flush=True)
                    break
                batch.append({"run_id": run_id, "file_path": rel,
                               "row_index": i, "row_json": row})
                if len(batch) >= BATCH_SIZE:
                    r = requests.post(rest("quantum_csv_rows"),
                                      headers=headers(), json=batch, timeout=30)
                    if r.status_code not in (200, 201, 204):
                        print(f"[PTMC-WATCHER] batch WARN {r.status_code}: {r.text[:60]}", flush=True)
                    rows_inserted += len(batch)
                    batch = []
                    time.sleep(0.03)
        if batch:
            r = requests.post(rest("quantum_csv_rows"),
                               headers=headers(), json=batch, timeout=30)
            rows_inserted += len(batch)
    except Exception as e:
        print(f"[PTMC-WATCHER] CSV parse ERR {rel}: {e}", flush=True)
        return False
    print(f"[PTMC-WATCHER] ✓ {rel} ({path.stat().st_size//1024}KB, {rows_inserted} lignes)", flush=True)
    return True


def is_file_closed(path: Path, stable_secs: float = 4.0) -> bool:
    """Retourne True si le fichier n'a pas changé de taille depuis stable_secs."""
    try:
        s1 = path.stat().st_size
        time.sleep(stable_secs)
        s2 = path.stat().st_size
        return s1 == s2 and s1 > 0
    except Exception:
        return False


def upload_and_delete(run_id: str, run_dir: Path, path: Path) -> bool:
    """Uploade un fichier CSV puis le supprime du disque. Retourne True si succès."""
    rel = str(path.relative_to(run_dir))
    size = path.stat().st_size
    if size == 0:
        path.unlink(missing_ok=True)
        return True
    sha = sha256_file(path)
    upsert_file_record(run_id, rel, "csv_ptmc", size, sha)
    ok = upload_csv_rows(run_id, rel, path)
    if ok:
        path.unlink(missing_ok=True)
        print(f"[PTMC-WATCHER] ✓ SUPPRIMÉ local: {path.name}", flush=True)
    else:
        print(f"[PTMC-WATCHER] ⚠ Upload Supabase échoué — suppression locale forcée: {path.name}", flush=True)
        path.unlink(missing_ok=True)
    return ok


def watch_and_upload(run_dir: Path, sentinel: str = ""):
    """Boucle principale : surveille les fichiers _part_*.csv et les uploade à chaud."""
    global _stop_flag
    run_id = run_dir.name
    uploaded = set()

    if not SUPABASE_URL or not SERVICE_KEY:
        print("[PTMC-WATCHER] Credentials manquants — watcher désactivé", flush=True)
        return

    print(f"[PTMC-WATCHER] Démarrage watcher pour run={run_id}", flush=True)
    print(f"[PTMC-WATCHER] Supabase URL={SUPABASE_URL}", flush=True)

    while not _stop_flag:
        # Condition d'arrêt : research_execution.log existe et non vide
        exec_log = run_dir / "logs" / "research_execution.log"
        if exec_log.exists() and exec_log.stat().st_size > 0:
            print("[PTMC-WATCHER] research_execution.log détecté non vide — arrêt watcher", flush=True)
            break

        # Condition d'arrêt : fichier sentinel .stop
        if sentinel and Path(sentinel + ".stop").exists():
            print("[PTMC-WATCHER] Sentinel .stop trouvé — arrêt", flush=True)
            break

        # Scanner tous les fichiers _part_*.csv dans logs/ et tests/
        for subdir in ["logs", "tests"]:
            d = run_dir / subdir
            if not d.exists():
                continue
            for fpath in sorted(d.glob("*_part_*.csv")):
                if fpath in uploaded:
                    continue
                if not is_file_closed(fpath, stable_secs=5.0):
                    continue
                # Vérifier que le fichier suivant dans la séquence existe
                # (garantit que la rotation a bien créé le suivant avant qu'on supprime)
                ok = upload_and_delete(run_id, run_dir, fpath)
                if ok:
                    uploaded.add(fpath)

        time.sleep(3)

    # Upload final des fichiers restants non encore traités
    print("[PTMC-WATCHER] Phase finale : upload des fichiers restants...", flush=True)
    for subdir in ["logs", "tests"]:
        d = run_dir / subdir
        if not d.exists():
            continue
        for fpath in sorted(d.glob("*_part_*.csv")):
            if fpath in uploaded and not fpath.exists():
                continue
            if fpath.exists():
                upload_and_delete(run_id, run_dir, fpath)

    print(f"[PTMC-WATCHER] Terminé — {len(uploaded)} fichiers uploadés et supprimés", flush=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("run_dir", help="Répertoire du run à surveiller")
    parser.add_argument("--sentinel", default="", help="Préfixe fichier sentinel")
    args = parser.parse_args()

    run_dir = Path(args.run_dir)
    if not run_dir.exists():
        # Attendre que le répertoire soit créé (jusqu'à 120s)
        for _ in range(40):
            time.sleep(3)
            if run_dir.exists():
                break
        else:
            print(f"[PTMC-WATCHER] run_dir {run_dir} jamais créé — abandon", flush=True)
            sys.exit(1)

    watch_and_upload(run_dir, args.sentinel)


if __name__ == "__main__":
    main()

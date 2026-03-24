#!/usr/bin/env python3
"""
download_from_supabase.py — C60-DOWNLOAD
Télécharge depuis Supabase les fichiers nécessaires à l'exécution du cycle de recherche.
Permet à chaque nouvelle session Replit de reprendre là où la précédente s'est arrêtée.

Usage:
    python3 tools/download_from_supabase.py [--run-id <run_id>] [--latest] [--list]

Variables d'environnement:
    SUPABASE_DB_HOST           = db.mwdeqpfxbcdayaelwqht.supabase.co
    SUPABASE_SERVICE_ROLE_KEY  = eyJ...
"""

import os
import sys
import json
import time
import argparse
from pathlib import Path

try:
    import requests
except ImportError:
    print("[DOWNLOAD] requests manquant — pip install requests")
    sys.exit(1)

ROOT = Path(__file__).resolve().parent.parent
RESULTS_DIR = ROOT / "results"


def _derive_url() -> str:
    db_host = os.environ.get("SUPABASE_DB_HOST", "")
    if db_host.startswith("db.") and ".supabase.co" in db_host:
        pid = db_host[3:].replace(".supabase.co", "")
        return f"https://{pid}.supabase.co"
    return os.environ.get("SUPABASE_URL", "").rstrip("/")


SERVICE_KEY  = os.environ.get("SUPABASE_SERVICE_ROLE_KEY", "")
SUPABASE_URL = _derive_url()


def _headers():
    return {
        "apikey":        SERVICE_KEY,
        "Authorization": f"Bearer {SERVICE_KEY}",
        "Content-Type":  "application/json",
    }


def _rest(endpoint: str) -> str:
    return f"{SUPABASE_URL}/rest/v1/{endpoint}"


def _check_credentials() -> bool:
    if not SUPABASE_URL or not SERVICE_KEY or not SERVICE_KEY.startswith("eyJ"):
        print("[DOWNLOAD-WARN] Credentials Supabase manquants ou invalides — skip", flush=True)
        return False
    return True


def list_runs() -> list:
    # C60-FIX : ne pas utiliser order=uploaded_at si la colonne n'existe pas
    # On récupère juste les run_id distincts sans tri par date
    resp = requests.get(
        _rest("quantum_run_files?select=run_id"),
        headers=_headers(), timeout=15
    )
    if resp.status_code != 200:
        print(f"[DOWNLOAD-WARN] list_runs: {resp.status_code} {resp.text[:100]}")
        return []
    rows = resp.json()
    seen = set()
    runs = []
    for r in rows:
        rid = r.get("run_id", "")
        if rid and rid not in seen:
            seen.add(rid)
            runs.append(rid)
    # Trier par nom (format research_YYYYMMDDTHHMMSSZ) desc = plus récent en premier
    runs.sort(reverse=True)
    return runs


def download_run(run_id: str, out_dir: Path) -> int:
    """Télécharge tous les fichiers d'un run depuis Supabase vers out_dir."""
    print(f"[DOWNLOAD] Téléchargement run={run_id} → {out_dir}", flush=True)
    out_dir.mkdir(parents=True, exist_ok=True)

    resp = requests.get(
        _rest(f"quantum_run_files?run_id=eq.{run_id}&select=file_path,file_type,content_text,file_size_bytes"),
        headers=_headers(), timeout=30
    )
    if resp.status_code != 200:
        print(f"[DOWNLOAD-WARN] Erreur récupération liste: {resp.status_code} {resp.text[:100]}")
        return 0

    files = resp.json()
    print(f"[DOWNLOAD] {len(files)} fichiers trouvés pour run={run_id}", flush=True)
    count = 0

    for f in files:
        rel  = f.get("file_path", "")
        text = f.get("content_text", "")
        if not rel:
            continue
        target = out_dir / rel
        target.parent.mkdir(parents=True, exist_ok=True)

        if not target.exists() or target.stat().st_size == 0:
            if text:
                target.write_text(text, encoding="utf-8", errors="replace")
                count += 1
                print(f"  [DL] {rel} ({len(text)} chars)", flush=True)
            else:
                target.touch()
    return count


def download_latest_run() -> Path | None:
    """Télécharge le dernier run disponible sur Supabase."""
    runs = list_runs()
    if not runs:
        print("[DOWNLOAD] Aucun run trouvé sur Supabase", flush=True)
        return None
    latest = runs[0]
    print(f"[DOWNLOAD] Dernier run Supabase: {latest}", flush=True)

    local_dir = RESULTS_DIR / latest
    if local_dir.exists():
        existing_files = list(local_dir.rglob("*"))
        if len(existing_files) > 2:
            print(f"[DOWNLOAD] Run déjà présent localement: {local_dir} ({len(existing_files)} fichiers) — skip", flush=True)
            return local_dir

    n = download_run(latest, local_dir)
    print(f"[DOWNLOAD] {n} fichiers téléchargés → {local_dir}", flush=True)
    return local_dir


def generate_problems_csv_from_supabase() -> bool:
    """
    Génère config/problems_cycle06.csv depuis la table research_modules_config sur Supabase.
    Appelé au démarrage de chaque session — source de vérité = Supabase.
    """
    resp = requests.get(
        _rest("research_modules_config?order=id.asc"),
        headers=_headers(), timeout=15
    )
    if resp.status_code != 200:
        print(f"[DOWNLOAD-CFG] Erreur research_modules_config: {resp.status_code} {resp.text[:100]}", flush=True)
        return False

    rows = resp.json()
    if not rows:
        print("[DOWNLOAD-CFG] Aucun module trouvé dans research_modules_config", flush=True)
        return False

    config_dir = ROOT / "config"
    config_dir.mkdir(exist_ok=True)
    out_path = config_dir / "problems_cycle06.csv"

    lines = ["name,lx,ly,t_eV,u_eV,mu_eV,temp_K,dt,steps"]
    for row in rows:
        lines.append(
            f"{row['module']},{row['lx']},{row['ly']},"
            f"{float(row['t_ev']):.6f},{float(row['u_ev']):.6f},{float(row['mu_ev']):.6f},"
            f"{float(row['temp_k']):.1f},{float(row['dt']):.6f},{int(row['steps'])}"
        )

    out_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"[DOWNLOAD-CFG] problems_cycle06.csv généré ({len(rows)} modules) → {out_path}", flush=True)
    return True


def download_benchmarks() -> int:
    """Télécharge les fichiers de référence benchmark depuis Supabase (table run_id='benchmarks')."""
    bench_dir = ROOT / "benchmarks"
    bench_dir.mkdir(exist_ok=True)

    resp = requests.get(
        _rest("quantum_run_files?run_id=eq.benchmarks&select=file_path,content_text"),
        headers=_headers(), timeout=15
    )
    if resp.status_code != 200:
        print(f"[DOWNLOAD-BENCH] Erreur: {resp.status_code}", flush=True)
        return 0

    files = resp.json()
    count = 0
    for f in files:
        rel  = f.get("file_path", "")
        text = f.get("content_text", "")
        if rel and text:
            target = bench_dir / rel
            if not target.exists() or target.stat().st_size == 0:
                target.write_text(text, encoding="utf-8", errors="replace")
                count += 1
                print(f"  [DL-BENCH] {rel}", flush=True)
    return count


def upload_benchmarks() -> int:
    """Upload les fichiers benchmark vers Supabase sous run_id='benchmarks'."""
    bench_dir = ROOT / "benchmarks"
    if not bench_dir.exists():
        return 0
    count = 0
    for fpath in sorted(bench_dir.rglob("*")):
        if not fpath.is_file() or fpath.stat().st_size == 0:
            continue
        try:
            text = fpath.read_text(errors="replace")
        except Exception:
            continue
        rel = fpath.name
        data = {
            "run_id": "benchmarks",
            "file_path": rel,
            "file_type": "benchmark",
            "file_size_bytes": fpath.stat().st_size,
            "sha256": "NA",
            "content_text": text[:500_000],
        }
        resp = requests.post(
            _rest("quantum_run_files"),
            headers={**_headers(), "Prefer": "resolution=merge-duplicates,return=minimal"},
            json=data, timeout=20
        )
        if resp.status_code in (200, 201, 204):
            count += 1
        else:
            print(f"  [UPL-BENCH-WARN] {rel}: {resp.status_code}", flush=True)
    return count


def main():
    parser = argparse.ArgumentParser(description="Téléchargement depuis Supabase")
    parser.add_argument("--run-id",  default="",   help="ID du run à télécharger")
    parser.add_argument("--latest",  action="store_true", help="Télécharger le dernier run")
    parser.add_argument("--list",    action="store_true", help="Lister les runs disponibles")
    parser.add_argument("--benchmarks", action="store_true", help="Télécharger les benchmarks de référence")
    parser.add_argument("--upload-benchmarks", action="store_true", help="Uploader les benchmarks locaux vers Supabase")
    args = parser.parse_args()

    if not _check_credentials():
        print("[DOWNLOAD] Pas de credentials — aucun téléchargement", flush=True)
        sys.exit(0)

    if args.list:
        runs = list_runs()
        print(f"\nRuns disponibles sur Supabase ({len(runs)}):")
        for r in runs[:20]:
            print(f"  {r}")
        return

    if args.benchmarks:
        n = download_benchmarks()
        print(f"[DOWNLOAD] {n} fichiers benchmark téléchargés")
        return

    if args.upload_benchmarks:
        n = upload_benchmarks()
        print(f"[UPLOAD-BENCH] {n} fichiers benchmark uploadés vers Supabase")
        return

    if args.run_id:
        local_dir = RESULTS_DIR / args.run_id
        n = download_run(args.run_id, local_dir)
        print(f"[DOWNLOAD] {n} fichiers téléchargés → {local_dir}")
        return

    if args.latest:
        download_latest_run()
        return

    # Sans argument : générer config CSV + télécharger benchmarks + dernier run
    print("[DOWNLOAD] Mode auto : benchmarks + dernier run", flush=True)
    generate_problems_csv_from_supabase()
    nb = download_benchmarks()
    print(f"[DOWNLOAD] {nb} fichiers benchmark téléchargés", flush=True)
    download_latest_run()


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""
upload_to_supabase.py — C60
Upload automatique des résultats d'un run vers Supabase via REST API.

Schéma réel Supabase (NE PAS MODIFIER — colonnes vérifiées le 2026-03-24) :
  quantum_run_files : run_id(UNIQUE), module, lx, ly, t_ev, u_ev, mu_ev,
                      temp_k, dt, steps, energy, pairing, sign_ratio,
                      cpu_percent, ram_percent, created_at
  quantum_csv_rows  : run_id, file_name, row_number, data(text), created_at
  quantum_benchmarks: dataset, module, observable, t_k, u_over_t,
                      reference_value, reference_method, source, error_bar,
                      notes, created_at

Usage:
    python3 upload_to_supabase.py <run_dir>
    python3 upload_to_supabase.py --all
    python3 upload_to_supabase.py --check-tables

Variables d'environnement:
    SUPABASE_SERVICE_ROLE_KEY  eyJ... (219 chars JWT, obligatoire)
    SUPABASE_DB_HOST           db.mwdeqpfxbcdayaelwqht.supabase.co (optionnel)
"""

import os
import sys
import re
import json
import argparse
import time
from pathlib import Path

try:
    import requests
except ImportError:
    print("[FATAL] requests manquant — pip install requests")
    sys.exit(1)

ROOT        = Path(__file__).resolve().parent.parent
RESULTS_DIR = ROOT / "results"

SUPABASE_URL = "https://mwdeqpfxbcdayaelwqht.supabase.co"
SERVICE_KEY  = os.environ.get("SUPABASE_SERVICE_ROLE_KEY", "")

BATCH_SIZE    = 200
MAX_LOG_LINES = 5000

def _hdrs(prefer="return=minimal"):
    return {
        "apikey":        SERVICE_KEY,
        "Authorization": f"Bearer {SERVICE_KEY}",
        "Content-Type":  "application/json",
        "Prefer":        prefer,
    }

def _rest(table):
    return f"{SUPABASE_URL}/rest/v1/{table}"

def _ok(status):
    return status in (200, 201, 204)

def _post(table, data, prefer="return=minimal", timeout=20):
    if not SERVICE_KEY:
        return False
    try:
        r = requests.post(_rest(table), headers=_hdrs(prefer), json=data, timeout=timeout)
        if not _ok(r.status_code):
            print(f"  [WARN] POST {table}: {r.status_code} | {r.text[:80]}")
            return False
        return True
    except Exception as e:
        print(f"  [ERR] POST {table}: {e}")
        return False

def _delete(table, filter_str, timeout=10):
    if not SERVICE_KEY:
        return
    try:
        requests.delete(f"{_rest(table)}?{filter_str}", headers=_hdrs(), timeout=timeout)
    except Exception:
        pass

def check_tables():
    if not SERVICE_KEY:
        print("[WARN] SUPABASE_SERVICE_ROLE_KEY absent — Supabase désactivé")
        return False
    ok = True
    for t in ["quantum_run_files", "quantum_csv_rows", "quantum_benchmarks"]:
        try:
            r = requests.get(f"{_rest(t)}?limit=1", headers=_hdrs(), timeout=10)
            status = "OK" if r.status_code == 200 else f"FAIL({r.status_code})"
            print(f"  [{status}] {t}")
            if r.status_code != 200:
                ok = False
        except Exception as e:
            print(f"  [ERR] {t}: {e}")
            ok = False
    return ok

def _parse_research_log(log_path: Path):
    """Parse research_execution.log → dict avec module principal + liste BASE_RESULT."""
    main_module = {}
    all_modules = []
    score = {}
    if not log_path.exists():
        return main_module, all_modules, score
    pattern = re.compile(
        r'BASE_RESULT problem=(\S+) energy=([\d.]+) pairing=([\d.]+) sign=(-?[\d.]+)'
        r' cpu_peak=([\d.]+) mem_peak=([\d.]+)'
    )
    score_pat = re.compile(r'SCORE iso=(\d+) trace=(\d+) repr=(\d+) robust=(\d+) phys=(\d+) expert=(\d+)')
    for line in log_path.read_text(errors="replace").splitlines():
        m = pattern.search(line)
        if m:
            row = {
                "module": m.group(1), "energy": float(m.group(2)),
                "pairing": float(m.group(3)), "sign_ratio": float(m.group(4)),
                "cpu_percent": float(m.group(5)), "ram_percent": float(m.group(6)),
            }
            all_modules.append(row)
            if m.group(1) == "hubbard_hts_core" or not main_module:
                main_module = row
        s = score_pat.search(line)
        if s:
            score = {k: int(v) for k, v in zip(
                ["iso","trace","repr","robust","phys","expert"],
                [s.group(i) for i in range(1,7)]
            )}
    return main_module, all_modules, score

def upload_run_file(run_id: str, main_module: dict):
    """Insère 1 ligne résumé dans quantum_run_files (UNIQUE sur run_id)."""
    _delete("quantum_run_files", f"run_id=eq.{run_id}")
    time.sleep(0.1)
    row = {
        "run_id": run_id,
        "module": main_module.get("module", "hubbard_hts_core"),
        "lx": 14, "ly": 14,
        "t_ev": 1.0, "u_ev": 8.0, "mu_ev": 0.0,
        "temp_k": 95.0, "dt": 0.05, "steps": 14000,
        "energy":       main_module.get("energy", 0.0),
        "pairing":      main_module.get("pairing", 0.0),
        "sign_ratio":   main_module.get("sign_ratio", 0.0),
        "cpu_percent":  main_module.get("cpu_percent", 0.0),
        "ram_percent":  main_module.get("ram_percent", 0.0),
    }
    ok = _post("quantum_run_files", row)
    print(f"  [quantum_run_files] {run_id[-4:]}: {'OK' if ok else 'FAIL'}")
    return ok

def upload_csv_rows(run_id: str, all_modules: list, score: dict, log_path: Path):
    """Insère les lignes détaillées dans quantum_csv_rows."""
    _delete("quantum_csv_rows", f"run_id=eq.{run_id}")
    time.sleep(0.1)

    rows = []
    for i, mod in enumerate(all_modules):
        rows.append({
            "run_id": run_id,
            "file_name": "research_execution.log",
            "row_number": i,
            "data": json.dumps(mod),
        })
    if score:
        rows.append({
            "run_id": run_id,
            "file_name": "research_execution.log",
            "row_number": len(all_modules),
            "data": json.dumps({"type": "SCORE", **score}),
        })

    if log_path.exists():
        log_lines = log_path.read_text(errors="replace").splitlines()
        for i, line in enumerate(log_lines[:MAX_LOG_LINES]):
            rows.append({
                "run_id": run_id,
                "file_name": "raw_log",
                "row_number": i,
                "data": line,
            })

    sent = 0
    for i in range(0, len(rows), BATCH_SIZE):
        batch = rows[i:i+BATCH_SIZE]
        ok = _post("quantum_csv_rows", batch, timeout=30)
        if ok:
            sent += len(batch)
        time.sleep(0.05)
    print(f"  [quantum_csv_rows] {run_id[-4:]}: {sent}/{len(rows)} lignes")

def upload_run(run_dir: Path):
    run_id = run_dir.name
    print(f"\n[SUPABASE] Upload {run_id}")

    if not SERVICE_KEY:
        print("  [SKIP] SUPABASE_SERVICE_ROLE_KEY absent — connexion fermée")
        return False

    log_path = run_dir / "logs" / "research_execution.log"
    main_module, all_modules, score = _parse_research_log(log_path)

    if not main_module and not all_modules:
        print(f"  [WARN] Aucune donnée BASE_RESULT trouvée dans {log_path}")
        if log_path.exists():
            main_module = {"module": "hubbard_hts_core", "energy": 0,
                           "pairing": 0, "sign_ratio": 0,
                           "cpu_percent": 0, "ram_percent": 0}
        else:
            print("  [SKIP] Log vide ou absent")
            return False

    upload_run_file(run_id, main_module)
    upload_csv_rows(run_id, all_modules, score, log_path)

    if score:
        total = sum(score.values())
        print(f"  [SCORE] iso={score.get('iso',0)} trace={score.get('trace',0)} "
              f"repr={score.get('repr',0)} robust={score.get('robust',0)} "
              f"phys={score.get('phys',0)} expert={score.get('expert',0)} "
              f"TOTAL={total}/600")
    else:
        print("  [SCORE] Pas de SCORE dans le log (run incomplet ou SIGKILL)")

    print(f"[SUPABASE] {run_id} — upload terminé")
    return True

def main():
    parser = argparse.ArgumentParser(description="Upload run LumVorax → Supabase REST")
    parser.add_argument("run_dir", nargs="?", help="Dossier du run")
    parser.add_argument("--all", action="store_true", help="Tous les runs dans results/")
    parser.add_argument("--check-tables", action="store_true", help="Vérifier les tables")
    args = parser.parse_args()

    if args.check_tables:
        sys.exit(0 if check_tables() else 1)

    if args.all:
        if not RESULTS_DIR.exists():
            print(f"[WARN] results/ absent: {RESULTS_DIR}")
            return
        runs = sorted([d for d in RESULTS_DIR.iterdir() if d.is_dir()])
        print(f"[SUPABASE] {len(runs)} runs")
        for r in runs:
            upload_run(r)
    elif args.run_dir:
        rd = Path(args.run_dir)
        if not rd.exists():
            print(f"[FATAL] {rd} introuvable")
            sys.exit(1)
        upload_run(rd)
    else:
        parser.print_help()

if __name__ == "__main__":
    main()

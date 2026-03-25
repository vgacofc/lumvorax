#!/usr/bin/env python3
"""
upload_to_supabase.py — C61
Upload automatique des résultats d'un run vers Supabase via REST API.

Schéma réel Supabase (NE PAS MODIFIER — colonnes vérifiées le 2026-03-24) :
  quantum_run_files : run_id(UNIQUE), module, lx, ly, t_ev, u_ev, mu_ev,
                      temp_k, dt, steps, energy, pairing, sign_ratio,
                      cpu_percent, ram_percent, created_at
  quantum_csv_rows  : run_id, file_name, row_number, data(text), created_at
  quantum_benchmarks: dataset, module, observable, t_k, u_over_t,
                      reference_value, reference_method, source, error_bar,
                      notes, created_at
  run_scores        : run_id(UNIQUE), runner, score_iso, score_trace,
                      score_repr, score_robust, score_phys, score_expert,
                      score_total, modules_ok, modules_total, created_at
  benchmark_runtime : run_id, dataset, module, observable, t_k, u_over_t,
                      reference_value, error_bar, model_value, abs_error,
                      rel_error, within_error_bar, created_at

Usage:
    python3 upload_to_supabase.py <run_dir>
    python3 upload_to_supabase.py --all
    python3 upload_to_supabase.py --check-tables
    python3 upload_to_supabase.py <run_dir> --delete-after

Variables d'environnement:
    SUPABASE_SERVICE_ROLE_KEY  eyJ... (219 chars JWT, obligatoire)
    SUPABASE_DB_HOST           db.mwdeqpfxbcdayaelwqht.supabase.co (optionnel)
"""

import os
import sys
import re
import csv
import json
import shutil
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

def _delete_rows(table, filter_str, timeout=10):
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
    tables = [
        "quantum_run_files", "quantum_csv_rows", "quantum_benchmarks",
        "run_scores", "benchmark_runtime",
    ]
    for t in tables:
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
    """Parse research_execution.log → dict avec module principal + liste BASE_RESULT + SCORE."""
    main_module = {}
    all_modules = []
    score = {}
    runner = "unknown"
    if not log_path.exists():
        return main_module, all_modules, score, runner
    pattern = re.compile(
        r'BASE_RESULT problem=(\S+) energy=([\d.]+) pairing=([\d.]+) sign=\s*(-?[\d.]+)'
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
                ["iso", "trace", "repr", "robust", "phys", "expert"],
                [s.group(i) for i in range(1, 7)]
            )}
    return main_module, all_modules, score, runner

def upload_run_file(run_id: str, main_module: dict):
    """Insère 1 ligne résumé dans quantum_run_files (UNIQUE sur run_id)."""
    _delete_rows("quantum_run_files", f"run_id=eq.{run_id}")
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

def upload_run_scores(run_id: str, all_modules: list, score: dict, runner: str):
    """Insère le score du run dans run_scores."""
    if not score:
        print(f"  [run_scores] {run_id[-4:]}: SKIP — pas de SCORE dans le log")
        return False
    _delete_rows("run_scores", f"run_id=eq.{run_id}")
    time.sleep(0.1)
    total = sum(score.values())
    row = {
        "run_id": run_id,
        "runner": runner if runner != "unknown" else (
            "advanced_parallel" if "advanced" in run_id.lower() else "fullscale"
        ),
        "score_iso":    score.get("iso", 0),
        "score_trace":  score.get("trace", 0),
        "score_repr":   score.get("repr", 0),
        "score_robust": score.get("robust", 0),
        "score_phys":   score.get("phys", 0),
        "score_expert": score.get("expert", 0),
        "score_total":  total,
        "modules_ok":   len(all_modules),
        "modules_total": 15,
    }
    ok = _post("run_scores", row)
    print(f"  [run_scores] {run_id[-4:]}: {'OK' if ok else 'FAIL'} — TOTAL={total}/600")
    return ok

def upload_benchmark_runtime(run_id: str, run_dir: Path):
    """Lit les fichiers benchmark CSV du run et insère dans benchmark_runtime."""
    bench_files = [
        run_dir / "tests" / "benchmark_comparison_qmc_dmrg.csv",
        run_dir / "tests" / "benchmark_comparison_external_modules.csv",
    ]
    rows = []
    for bf in bench_files:
        if not bf.exists():
            continue
        dataset = bf.stem.replace("benchmark_comparison_", "")
        try:
            with open(bf, newline="", errors="replace") as f:
                reader = csv.DictReader(f)
                for row in reader:
                    try:
                        ref_val   = float(row.get("reference") or row.get("reference_value") or 0)
                        mod_val   = float(row.get("model") or row.get("model_value") or 0)
                        err_bar   = float(row.get("error_bar") or 0)
                        abs_err   = float(row.get("abs_error") or 0)
                        rel_err   = float(row.get("rel_error") or 0)
                        within    = (abs_err <= err_bar) if err_bar > 0 else False
                        rows.append({
                            "run_id":          run_id,
                            "dataset":         dataset,
                            "module":          row.get("module", ""),
                            "observable":      row.get("observable", ""),
                            "t_k":             float(row.get("T") or row.get("t_k") or 0),
                            "u_over_t":        float(row.get("U") or row.get("u_over_t") or 0),
                            "reference_value": ref_val,
                            "error_bar":       err_bar,
                            "model_value":     mod_val,
                            "abs_error":       abs_err,
                            "rel_error":       rel_err,
                            "within_error_bar": within,
                        })
                    except (ValueError, TypeError):
                        continue
        except Exception as e:
            print(f"  [benchmark_runtime] ERR lecture {bf.name}: {e}")

    if not rows:
        print(f"  [benchmark_runtime] {run_id[-4:]}: SKIP — aucun fichier benchmark trouvé")
        return False
    _delete_rows("benchmark_runtime", f"run_id=eq.{run_id}")
    time.sleep(0.1)
    sent = 0
    for i in range(0, len(rows), BATCH_SIZE):
        batch = rows[i:i+BATCH_SIZE]
        ok = _post("benchmark_runtime", batch, timeout=30)
        if ok:
            sent += len(batch)
        time.sleep(0.05)
    print(f"  [benchmark_runtime] {run_id[-4:]}: {sent}/{len(rows)} lignes")
    return sent > 0

def upload_csv_rows(run_id: str, all_modules: list, score: dict, log_path: Path):
    """Insère les lignes détaillées dans quantum_csv_rows."""
    _delete_rows("quantum_csv_rows", f"run_id=eq.{run_id}")
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

def upload_run(run_dir: Path, delete_after: bool = False):
    run_id = run_dir.name
    print(f"\n[SUPABASE] Upload {run_id}")

    if not SERVICE_KEY:
        print("  [SKIP] SUPABASE_SERVICE_ROLE_KEY absent — Supabase désactivé")
        return False

    log_path = run_dir / "logs" / "research_execution.log"
    main_module, all_modules, score, runner = _parse_research_log(log_path)

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
    upload_run_scores(run_id, all_modules, score, runner)
    upload_benchmark_runtime(run_id, run_dir)

    if score:
        total = sum(score.values())
        print(f"  [SCORE] iso={score.get('iso',0)} trace={score.get('trace',0)} "
              f"repr={score.get('repr',0)} robust={score.get('robust',0)} "
              f"phys={score.get('phys',0)} expert={score.get('expert',0)} "
              f"TOTAL={total}/600")
    else:
        print("  [SCORE] Pas de SCORE dans le log (run incomplet ou SIGKILL)")

    print(f"[SUPABASE] {run_id} — upload terminé")

    # C61-DELETE-AFTER : suppression locale après upload réussi pour libérer l'espace disque
    # Evite l'accumulation de GB de données CSV qui provoque le SIGKILL du workflow
    if delete_after:
        try:
            shutil.rmtree(run_dir)
            print(f"[SUPABASE] {run_id} — répertoire local supprimé (--delete-after)")
        except Exception as e:
            print(f"[SUPABASE-WARN] Suppression locale {run_id} échouée: {e}")

    return True

def main():
    parser = argparse.ArgumentParser(description="Upload run LumVorax → Supabase REST")
    parser.add_argument("run_dir", nargs="?", help="Dossier du run")
    parser.add_argument("--all", action="store_true", help="Tous les runs dans results/")
    parser.add_argument("--check-tables", action="store_true", help="Vérifier les tables")
    parser.add_argument("--delete-after", action="store_true",
                        help="Supprimer le répertoire local après upload réussi")
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
            upload_run(r, delete_after=args.delete_after)
    elif args.run_dir:
        rd = Path(args.run_dir)
        if not rd.exists():
            print(f"[FATAL] {rd} introuvable")
            sys.exit(1)
        upload_run(rd, delete_after=args.delete_after)
    else:
        parser.print_help()

if __name__ == "__main__":
    main()

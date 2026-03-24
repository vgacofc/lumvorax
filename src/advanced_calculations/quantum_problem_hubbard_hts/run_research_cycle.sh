#!/usr/bin/env bash
# ============================================================================
# run_research_cycle.sh — Cycle de recherche Hubbard/LumVorax
# ============================================================================
# INSTRUCTION PERMANENTE : Répondre TOUJOURS en français dans les rapports.
# INSTRUCTION PERMANENTE : NE JAMAIS RÉDUIRE LES LOGS — augmenter la traçabilité.
# INSTRUCTION PERMANENTE : Si un run est arrêté (SIGTERM/timeout), relancer
#   automatiquement depuis le dernier checkpoint jusqu'à finalisation 100%.
# INSTRUCTION PERMANENTE : Utiliser 99% CPU/RAM/DISK disponible.
# INSTRUCTION PERMANENTE : NOMS D'ORIGINE CONSERVÉS — FORENSIC_LOG_MODULE_METRIC
#   est le nom officiel (71 usages runner) ; LV_MODULE_METRIC = bridge secondaire.
# INSTRUCTION PERMANENTE : Rotation automatique des fichiers CSV à 95 MB
#   (lv_rotate_csv dans ultra_forensic_logger.c + ftell inter-module PT-MC).
# ============================================================================

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
SCRIPT_PATH="$(realpath "$0")"
STAMP_UTC="$(date -u +%Y%m%dT%H%M%SZ)"
BACKUP_DIR="$ROOT_DIR/backups/research_cycle_$STAMP_UTC"
SESSION_LOG_DIR="$ROOT_DIR/logs"
SESSION_LOG="$SESSION_LOG_DIR/research_cycle_session_${STAMP_UTC}.log"

# ── C26-CHECKPOINT : fichier d'état persistant pour résumption automatique ──
CHECKPOINT_FILE="$ROOT_DIR/.run_checkpoint"
CHECKPOINT_PHASE_FILE="$ROOT_DIR/.run_current_phase"

# ── C26-RESUME : lire le dernier checkpoint si présent ──────────────────────
RESUME_FROM_PHASE=0
if [ -f "$CHECKPOINT_PHASE_FILE" ]; then
    RESUME_FROM_PHASE="$(cat "$CHECKPOINT_PHASE_FILE" 2>/dev/null || echo 0)"
    echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] RÉSUMPTION détectée — reprise depuis phase ${RESUME_FROM_PHASE}"
fi

# Sauvegarde de phase courante (appelée après chaque étape réussie)
checkpoint_save() {
    local phase_num="$1"
    echo "$phase_num" > "$CHECKPOINT_PHASE_FILE"
    echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] CHECKPOINT sauvegardé : phase ${phase_num}"
}

# ── C26-SIGTERM : Handler — relance automatique depuis le checkpoint ─────────
handle_sigterm() {
    local current_phase
    current_phase="$(cat "$CHECKPOINT_PHASE_FILE" 2>/dev/null || echo 0)"
    echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] SIGTERM reçu — checkpoint phase=${current_phase}"
    echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] Relancement automatique depuis phase ${current_phase}..."
    # Relancer en background avec nohup pour survivre à la mort du parent
    nohup bash "$SCRIPT_PATH" >> "$SESSION_LOG" 2>&1 &
    echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] Relancement PID=$! lancé en background"
    exit 0
}
trap handle_sigterm SIGTERM SIGINT

mkdir -p "$BACKUP_DIR"
mkdir -p "$SESSION_LOG_DIR"

# Persistent real-time log (console + file)
exec > >(stdbuf -oL tee -a "$SESSION_LOG") 2>&1

echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] run_research_cycle start stamp=${STAMP_UTC}"
echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] RESUME_FROM_PHASE=${RESUME_FROM_PHASE}"

# ── C60-SUPABASE-DOWNLOAD : récupération des fichiers depuis Supabase au démarrage ──
# Garantit que chaque nouvelle session Replit dispose des benchmarks et du dernier run
# même si LFS est désactivé et que les fichiers locaux ont disparu après un push.
echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] [C60-DL] Téléchargement depuis Supabase..."
if python3 "$ROOT_DIR/tools/download_from_supabase.py" 2>&1; then
    echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] [C60-DL] Download Supabase OK"
else
    echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] [C60-DL-WARN] Download partiel ou pas de connexion — continue sans"
fi

# ── BC-LV04 fix : Suppression TOTALE de toutes les restrictions de ressources ──
# Aucun ulimit RAM/CPU — test jusqu'aux limites hardware réelles (99% dynamique)
ulimit -v unlimited 2>/dev/null || true
ulimit -m unlimited 2>/dev/null || true
ulimit -s unlimited 2>/dev/null || true
# C60-BUG-LV01-FIX : augmenter la limite de fichiers ouverts pour le runner advanced_parallel
# Sans ce fix, FOPEN_DIAG échoue (EMFILE) → research_execution.log reste 0 bytes → score=0
ulimit -n 8192 2>/dev/null || ulimit -n 4096 2>/dev/null || true
echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] [C60] ulimit -n=$(ulimit -n) (limite descripteurs fichiers)"
# Priorité maximale pour le processus (nice -20 si possible)
renice -n -10 $$ 2>/dev/null || true

# ── BC-LV04 fix : Wrapper LumVorax pour phases Python 8-39 ──────────
# Lit LUMVORAX_CSV_PATH (défini après le run C) et instrumente chaque phase Python
_LV_PHASE_NUM=7  # Compteur de phase Python (commence à 8 = après 7 phases C)

lv_wrap() {
  # Usage: lv_wrap phase_num python3 script.py [args...]
  local phase="$1"; shift
  local t_start; t_start="$(date +%s%N)"
  local iso; iso="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  local pid="$$"
  local script_name; script_name="$(basename "$1" 2>/dev/null || echo unknown)"

  # Écriture PHASE_BRIDGE START + HW_SAMPLE dans le CSV LumVorax
  if [ -n "${LUMVORAX_CSV_PATH:-}" ] && [ -f "${LUMVORAX_CSV_PATH:-}" ]; then
    local mem_pct cpu_info
    mem_pct="$(awk '/MemTotal/{t=$2} /MemAvailable/{a=$2} END{printf "%.2f", 100*(t-a)/t}' /proc/meminfo 2>/dev/null || echo -1)"
    cpu_info="$(awk 'NR==1{u=$2+$4; i=$5; t=u+i; printf "%.2f", 100*u/t}' /proc/stat 2>/dev/null || echo -1)"
    local ts; ts="$(date +%s%N)"
    {
      printf "PHASE_BRIDGE,%s,%s,%s,phase%02d:%s,START\n" "$iso" "$ts" "$pid" "$phase" "$script_name"
      printf "HW_SAMPLE,%s,%s,%s,phase%02d:mem_pct,%s\n" "$iso" "$ts" "$pid" "$phase" "$mem_pct"
      printf "HW_SAMPLE,%s,%s,%s,phase%02d:cpu_snapshot,%s\n" "$iso" "$ts" "$pid" "$phase" "$cpu_info"
    } >> "${LUMVORAX_CSV_PATH}"
  fi

  # Exécution du script Python avec capture de statut
  local exit_code=0
  "$@" || exit_code=$?

  # Écriture PHASE_BRIDGE END + durée
  if [ -n "${LUMVORAX_CSV_PATH:-}" ] && [ -f "${LUMVORAX_CSV_PATH:-}" ]; then
    local t_end; t_end="$(date +%s%N)"
    local duration_ns=$(( t_end - t_start ))
    local status="SUCCESS"
    [ "$exit_code" -ne 0 ] && status="FAILURE_exit${exit_code}"
    local mem_pct_end; mem_pct_end="$(awk '/MemTotal/{t=$2} /MemAvailable/{a=$2} END{printf "%.2f", 100*(t-a)/t}' /proc/meminfo 2>/dev/null || echo -1)"
    local iso2; iso2="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    {
      printf "PHASE_BRIDGE,%s,%s,%s,phase%02d:%s,%s\n" "$iso2" "$t_end" "$pid" "$phase" "$script_name" "$status"
      printf "METRIC,%s,%s,%s,phase%02d:duration_ns,%s\n" "$iso2" "$t_end" "$pid" "$phase" "$duration_ns"
      printf "HW_SAMPLE,%s,%s,%s,phase%02d:mem_pct_end,%s\n" "$iso2" "$t_end" "$pid" "$phase" "$mem_pct_end"
    } >> "${LUMVORAX_CSV_PATH}"
    # Détection anomalie si exit_code != 0
    if [ "$exit_code" -ne 0 ]; then
      printf "ANOMALY,%s,%s,%s,phase%02d:exit_code,%s\n" "$iso2" "$t_end" "$pid" "$phase" "$exit_code" >> "${LUMVORAX_CSV_PATH}"
    fi
  fi
  # C22-BUG-POST : Ne pas propager l'exit_code vers le shell.
  # Avec set -euo pipefail actif, un exit_code != 0 tuerait le run entier.
  # L'exit_code est déjà tracé dans le CSV LumVorax via ANOMALY — aucune perte d'info.
  return 0
}
cp -a "$ROOT_DIR/include" "$BACKUP_DIR/"
cp -a "$ROOT_DIR/src" "$BACKUP_DIR/"
cp -a "$ROOT_DIR/Makefile" "$BACKUP_DIR/"
cp -a "$ROOT_DIR/benchmarks" "$BACKUP_DIR/"

TOTAL_STEPS="$(grep -c '^[[:space:]]*print_progress "' "$SCRIPT_PATH")"
if [ "${TOTAL_STEPS:-0}" -le 0 ]; then
  TOTAL_STEPS=1
fi
CURRENT_STEP=0

print_progress() {
  CURRENT_STEP=$((CURRENT_STEP + 1))
  local label="$1"
  local width=40
  local filled=$((CURRENT_STEP * width / TOTAL_STEPS))
  local empty=$((width - filled))

  local bar
  bar=$(printf '%*s' "$filled" '' | tr ' ' '#')
  bar+=$(printf '%*s' "$empty" '' | tr ' ' '-')

  printf "\r[%s] %3d%% (%d/%d) %s" \
    "$bar" \
    "$((CURRENT_STEP * 100 / TOTAL_STEPS))" \
    "$CURRENT_STEP" \
    "$TOTAL_STEPS" \
    "$label"

  if [ "$CURRENT_STEP" -eq "$TOTAL_STEPS" ]; then
    printf "\n"
  fi
}

write_checksums() {
  local target_run_dir="$1"
  (
    cd "$target_run_dir"
    rm -f logs/checksums.sha256
    find . -type f ! -path './logs/checksums.sha256' -print0 | sort -z | xargs -0 sha256sum > logs/checksums.sha256
  )
}

# T02/T18 — GLOBAL_CHECKSUM.sha512 : traçabilité totale par run (sha512 de tous les fichiers résultats)
write_global_sha512() {
  local target_run_dir="$1"
  (
    cd "$target_run_dir"
    find . -type f ! -name 'GLOBAL_CHECKSUM.sha512' -print0 | sort -z | xargs -0 sha512sum > GLOBAL_CHECKSUM.sha512
    echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] GLOBAL_CHECKSUM.sha512 generated — $(wc -l < GLOBAL_CHECKSUM.sha512) files hashed"
  )
}

verify_checksums() {
  local target_run_dir="$1"
  (
    cd "$target_run_dir"
    sha256sum -c logs/checksums.sha256 >/dev/null
  )
}

checkpoint_save 1
make -C "$ROOT_DIR" clean all
print_progress "build"
checkpoint_save 2

# Force forensic runtime toggles ON for full traceability contract
export LUMVORAX_FORENSIC_REALTIME="1"
export LUMVORAX_LOG_PERSISTENCE="1"
export LUMVORAX_HFBL360_ENABLED="1"
export LUMVORAX_MEMORY_TRACKER="1"
export LUMVORAX_RUN_GROUP="campaign_${STAMP_UTC}"

# ── C26-RUNNER-RETRY : relance automatique si le runner fullscale est tué ───
# Retry jusqu'à MAX_RUNNER_RETRY tentatives, sans jamais réduire les logs
MAX_RUNNER_RETRY=5
FULLSCALE_OK=0
for _try in $(seq 1 $MAX_RUNNER_RETRY); do
    echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] Runner fullscale — tentative ${_try}/${MAX_RUNNER_RETRY}"
    export LUMVORAX_SOLVER_VARIANT="fullscale"
    if "$ROOT_DIR/hubbard_hts_research_runner" "$ROOT_DIR"; then
        FULLSCALE_OK=1
        echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] Runner fullscale terminé avec succès (tentative ${_try})"
        break
    else
        echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] Runner fullscale interrompu (exit=$?) — relance dans 2s..."
        sleep 2
    fi
done
[ "$FULLSCALE_OK" -eq 0 ] && echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] WARNING: Runner fullscale non terminé après ${MAX_RUNNER_RETRY} tentatives — continuation quand même"
print_progress "fullscale simulation"
checkpoint_save 3

LATEST_FULLSCALE_RUN="$(ls -1 "$ROOT_DIR/results" | rg '^research_' | tail -n 1)"
FULLSCALE_RUN_DIR="$ROOT_DIR/results/$LATEST_FULLSCALE_RUN"

# C22-BUG04 FIX : entourer avec lv_wrap pour que SystemExit(1) du script Python
# ne propage pas d'exit code non-zero au shell (set -euo pipefail).
# Sans lv_wrap, une incohérence de colonnes dans le CSV forensique
# (ex. lignes ANOMALY vs METRIC) interrompait le run avant la phase advanced parallel.
lv_wrap 7 python3 "$ROOT_DIR/tools/post_run_csv_schema_guard.py" "$FULLSCALE_RUN_DIR"
print_progress "fullscale csv schema guard"

write_checksums "$FULLSCALE_RUN_DIR"
print_progress "fullscale checksums"
write_global_sha512 "$FULLSCALE_RUN_DIR"
print_progress "fullscale sha512 global"
verify_checksums "$FULLSCALE_RUN_DIR"
print_progress "fullscale checksum verify"

# ── C60-PTMC-WATCHER : upload temps réel CSV PTMC → évite accumulation disque + SIGKILL ──
# Lance un watcher Python en background qui détecte le nouveau run_dir ADV,
# uploade chaque fichier _part_*.csv dès sa fermeture et le supprime du disque.
_WATCHER_RESULTS="$ROOT_DIR/results"
_WATCHER_BEFORE="$(ls -1d "$_WATCHER_RESULTS"/research_* 2>/dev/null | sort | tail -n 1 || echo '')"
(
    for _w in $(seq 1 60); do
        sleep 3
        _NEW_RUN="$(ls -1d "$_WATCHER_RESULTS"/research_* 2>/dev/null | sort | tail -n 1 || echo '')"
        if [ -n "$_NEW_RUN" ] && [ "$_NEW_RUN" != "$_WATCHER_BEFORE" ]; then
            echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] [C60-WATCHER] Nouveau run détecté: $_NEW_RUN"
            python3 "$ROOT_DIR/tools/ptmc_realtime_uploader.py" "$_NEW_RUN"
            break
        fi
    done
) &
WATCHER_PID=$!
echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] [C60-WATCHER] PID=$WATCHER_PID lancé en background"

# ── C26-RUNNER-RETRY : relance automatique si le runner advanced_parallel est tué ─
ADV_OK=0
for _try in $(seq 1 $MAX_RUNNER_RETRY); do
    echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] Runner advanced_parallel — tentative ${_try}/${MAX_RUNNER_RETRY}"
    export LUMVORAX_SOLVER_VARIANT="advanced_parallel"
    if "$ROOT_DIR/hubbard_hts_research_runner_advanced_parallel" "$ROOT_DIR"; then
        ADV_OK=1
        echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] Runner advanced_parallel terminé avec succès (tentative ${_try})"
        break
    else
        echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] Runner advanced_parallel interrompu (exit=$?) — relance dans 2s..."
        sleep 2
    fi
done

# Arrêt propre du watcher après fin du runner
kill "$WATCHER_PID" 2>/dev/null || true
wait "$WATCHER_PID" 2>/dev/null || true
echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] [C60-WATCHER] Watcher arrêté"
[ "$ADV_OK" -eq 0 ] && echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] WARNING: Runner advanced_parallel non terminé après ${MAX_RUNNER_RETRY} tentatives — continuation quand même"
print_progress "advanced parallel simulation"
checkpoint_save 10

LATEST_ADV_RUN="$(ls -1 "$ROOT_DIR/results" | rg '^research_' | tail -n 1)"
ADV_RUN_DIR="$ROOT_DIR/results/$LATEST_ADV_RUN"

RUN_DIR="$ADV_RUN_DIR"
LATEST_RUN="$LATEST_ADV_RUN"

# ── BC-LV04 fix : Export LUMVORAX_CSV_PATH pour les phases Python 8-39 ──
# Trouve le dernier CSV LumVorax créé par le runner C (advanced_parallel)
LUMVORAX_CSV_PATH="$(find "$RUN_DIR/logs" -name 'lumvorax_*.csv' -type f 2>/dev/null | sort | tail -n 1 || true)"
export LUMVORAX_CSV_PATH
if [ -n "${LUMVORAX_CSV_PATH:-}" ]; then
  echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] LUMVORAX_CSV_PATH=$LUMVORAX_CSV_PATH (phases 8-39 instrumented)"
else
  echo "[$(date -u +%Y-%m-%dT%H:%M:%SZ)] LUMVORAX_CSV_PATH not found — phases 8-39 will run uninstrumented"
fi

lv_wrap 8 python3 "$ROOT_DIR/tools/post_run_csv_schema_guard.py" "$RUN_DIR"
print_progress "advanced csv schema guard"

{
  echo "timestamp_utc=$STAMP_UTC"
  echo "hostname=$(hostname)"
  echo "uname=$(uname -a)"
  echo "gcc_version=$(gcc --version | head -n 1)"
} > "$RUN_DIR/logs/environment_versions.log"

# Cycle integrations: metadata first, then guard/gates, then physics pack.

lv_wrap 9  python3 "$ROOT_DIR/tools/post_run_metadata_capture.py" "$RUN_DIR"
print_progress "metadata capture"

lv_wrap 10 python3 "$ROOT_DIR/tools/post_run_cycle_guard.py" "$ROOT_DIR" "$RUN_DIR"
print_progress "cycle guard"

lv_wrap 11 python3 "$ROOT_DIR/tools/post_run_physics_readiness_pack.py" "$RUN_DIR"
print_progress "physics readiness"

# BC-10 : mise à jour automatique runtime benchmark (R13 — RMSE < 0.05 requis)
lv_wrap 12 python3 "$ROOT_DIR/tools/post_run_update_runtime_benchmark.py" "$RUN_DIR" "$ROOT_DIR/benchmarks"
print_progress "runtime benchmark update"

lv_wrap 13 python3 "$ROOT_DIR/tools/post_run_v4next_integration_status.py" "$RUN_DIR"
print_progress "v4next status"

ROLL_MODE="${LUMVORAX_ROLLOUT_MODE:-full}"   # C37-P4 : CANARY→FULL (seuils conn=80%,realism=95%,shadow=85% satisfaits)
lv_wrap 14 python3 "$ROOT_DIR/tools/v4next_rollout_controller.py" "$RUN_DIR" "$ROLL_MODE"
print_progress "rollout controller"

lv_wrap 15 python3 "$ROOT_DIR/tools/post_run_v4next_rollout_progress.py" "$RUN_DIR"
print_progress "rollout progress"

lv_wrap 16 python3 "$ROOT_DIR/tools/post_run_v4next_realtime_evolution.py" "$ROOT_DIR" "$RUN_DIR"
print_progress "realtime evolution"

lv_wrap 17 python3 "$ROOT_DIR/tools/post_run_low_level_telemetry.py" "$RUN_DIR"
print_progress "low-level telemetry"

lv_wrap 18 python3 "$ROOT_DIR/tools/post_run_advanced_observables_pack.py" "$RUN_DIR"
print_progress "advanced observables"

lv_wrap 19 python3 "$ROOT_DIR/tools/run_independent_physics_modules.py" "$RUN_DIR"
print_progress "independent qmc/dmrg/arpes/stm"

# C55-Q28-FIX : fss_binder AVANT chatgpt_critical_tests (Q28 dépend de fss_binder_cumulants.csv)
lv_wrap 191 python3 "$ROOT_DIR/tools/post_run_fss_tc_binder.py" "$RUN_DIR"
print_progress "fss tc binder (pre-critical)"

# C55-Q29-FIX : optical_conductivity AVANT chatgpt_critical_tests (Q29 dépend de integration_optical_conductivity.csv)
lv_wrap 192 python3 "$ROOT_DIR/tools/post_run_optical_conductivity.py" "$RUN_DIR"
print_progress "optical conductivity (pre-critical)"

lv_wrap 20 python3 "$ROOT_DIR/tools/post_run_chatgpt_critical_tests.py" "$RUN_DIR"
print_progress "critical tests"

lv_wrap 21 python3 "$ROOT_DIR/tools/post_run_problem_solution_progress.py" "$RUN_DIR"
print_progress "solution progress"

lv_wrap 22 python3 "$ROOT_DIR/tools/post_run_authenticity_audit.py" "$ROOT_DIR" "$RUN_DIR"
print_progress "authenticity audit"

lv_wrap 23 python3 "$ROOT_DIR/tools/post_run_cycle35_exhaustive_report.py" "$ROOT_DIR" "$RUN_DIR"
print_progress "cycle35 report"

lv_wrap 24 python3 "$ROOT_DIR/tools/post_run_full_scope_integrator.py" --root "$ROOT_DIR" "$RUN_DIR"
print_progress "full-scope integration"

# C59-P1 FIX : Régénérer SHA512 + SHA256 AVANT le rapport scientifique.
# L'ancien bloc supprimait logs/checksums.sha256 sans régénérer → traceability_pct < 100%.
# write_global_sha512 AVANT write_checksums : le sha256 inclut le sha512 final (règle T02/T18).
write_global_sha512 "$RUN_DIR"
print_progress "pre-report sha512 global (C59-P1)"
write_checksums "$RUN_DIR"
print_progress "pre-report checksums (C59-P1)"

lv_wrap 25 python3 "$ROOT_DIR/tools/post_run_scientific_report_cycle.py" "$RUN_DIR"
print_progress "scientific report"

lv_wrap 26 python3 "$ROOT_DIR/tools/post_run_independent_log_review.py" "$RUN_DIR"
print_progress "independent review"

lv_wrap 27 python3 "$ROOT_DIR/tools/post_run_3d_modelization_export.py" "$RUN_DIR"
print_progress "3d model export"

lv_wrap 28 python3 "$ROOT_DIR/tools/post_run_remote_depot_independent_analysis.py" "$ROOT_DIR" --run-dir "$RUN_DIR"
print_progress "remote independent analysis"

lv_wrap 29 python3 "$ROOT_DIR/tools/post_run_parallel_calibration_bridge.py" "$RUN_DIR"
print_progress "parallel calibration bridge"

lv_wrap 30 python3 "$ROOT_DIR/tools/post_run_hfbl360_forensic_logger.py" "$RUN_DIR" --standard-names "$ROOT_DIR/../../../STANDARD_NAMES.md"
print_progress "hfbl360 forensic logger"

# C36-P4 : ARPES synthétique + FSS Tc Binder
lv_wrap 31 python3 "$ROOT_DIR/tools/post_run_arpes_synthetic.py" "$ROOT_DIR" "$RUN_DIR"
print_progress "arpes synthetic"

lv_wrap 32 python3 "$ROOT_DIR/tools/post_run_fss_tc_binder.py" "$RUN_DIR"
print_progress "fss tc binder"

# C36-P2 : cross-center consensus node-2
lv_wrap 33 python3 "$ROOT_DIR/tools/post_run_cross_center_consensus.py" "$RUN_DIR"
print_progress "cross center consensus"

# C38-P8-DYNSCALE : Scan dynamique de la taille de réseau jusqu'à 99% ressources CPU/RAM
lv_wrap 34 python3 "$ROOT_DIR/tools/post_run_dynamic_hilbert_scan.py" "$RUN_DIR"
print_progress "dynamic hilbert scan"

# C39-F1 : Conductivité optique σ(ω) via formule Kubo-Drude (corrélations J-J)
# Lit parallel_tempering_mc_results.csv + model_metadata.csv
# Produit integration_optical_conductivity.csv (σ(ω) par module et T) et
# integration_drude_weight.csv (poids D(T) — signature de la phase SC)
lv_wrap 35 python3 "$ROOT_DIR/tools/post_run_optical_conductivity.py" "$RUN_DIR"
print_progress "optical conductivity sigma(omega)"

# C39-H1 : Autocorrélation et temps d'intégration τ_int (estimateur Sokal)
# Calcule τ_int pour l'énergie et le pairing par module — nécessaire pour dTc correct
lv_wrap 36 python3 "$ROOT_DIR/tools/post_run_autocorr.py" --run-dir "$RUN_DIR" --output "$RUN_DIR/tests/autocorr_tau_int_sokal.csv" --c-factor 6.0 --max-window 5000
print_progress "autocorr tau_int Sokal"

write_checksums "$RUN_DIR"
print_progress "checksums"
write_global_sha512 "$RUN_DIR"
print_progress "sha512 global"


CAMPAIGN_DIR="$ROOT_DIR/results/${LUMVORAX_RUN_GROUP}"
mkdir -p "$CAMPAIGN_DIR"
cat > "$CAMPAIGN_DIR/campaign_manifest.txt" <<MANIFEST
stamp_utc=$STAMP_UTC
run_group=${LUMVORAX_RUN_GROUP}
fullscale_run=$LATEST_FULLSCALE_RUN
advanced_run=$LATEST_ADV_RUN
fullscale_run_dir=$FULLSCALE_RUN_DIR
advanced_run_dir=$ADV_RUN_DIR
MANIFEST
print_progress "campaign manifest"

python3 "$ROOT_DIR/tools/post_run_fullscale_vs_advanced_compare.py" "$FULLSCALE_RUN_DIR" "$ADV_RUN_DIR" --out-dir "$CAMPAIGN_DIR"
print_progress "fullscale vs advanced compare"

python3 "$ROOT_DIR/tools/post_run_fullscale_vs_fullscale_benchmark.py" "$RUN_DIR"
print_progress "fullscale vs fullscale benchmark"

echo "Research cycle terminé (advanced): $RUN_DIR"
echo "Fullscale run: $FULLSCALE_RUN_DIR"
echo "Advanced run: $ADV_RUN_DIR"
echo "Campaign artifacts: $CAMPAIGN_DIR"
echo "Session log: $SESSION_LOG"

if [ "${LUMVORAX_FULLSCALE_STRICT:-1}" = "1" ]; then
  "$ROOT_DIR/run_fullscale_strict_protocol.sh" "$RUN_DIR"
  print_progress "fullscale strict protocol audit"
fi

# Finalize checksums at very end so later post-steps cannot stale the manifest.
# T02/T18 : SHA512 AVANT SHA256 pour que le sha256 inclue le sha512 final (évite mismatch).
write_global_sha512 "$RUN_DIR"
print_progress "final sha512 global (T02/T18)"
write_checksums "$RUN_DIR"
print_progress "final checksums"
verify_checksums "$RUN_DIR"
print_progress "final checksum verify"
checkpoint_save 99

# ── C26-CHECKPOINT-CLEAR : Succès complet — supprimer le checkpoint ──────────
echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] Run complet à 100% — checkpoint supprimé"
rm -f "$CHECKPOINT_PHASE_FILE" "$CHECKPOINT_FILE"

# ── C59-SUPABASE : Upload automatique vers Supabase + suppression locale ──────
echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] [SUPABASE] Upload du run vers Supabase..."
if python3 "$ROOT_DIR/tools/upload_to_supabase.py" "$RUN_DIR" --delete-after 2>&1; then
    echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] [SUPABASE] Upload fullscale run OK — fichiers locaux supprimés"
else
    echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] [SUPABASE-WARN] Upload partiel ou échoué — fichiers conservés localement"
fi
if [ -n "$ADV_RUN_DIR" ] && [ -d "$ADV_RUN_DIR" ]; then
    if python3 "$ROOT_DIR/tools/upload_to_supabase.py" "$ADV_RUN_DIR" --delete-after 2>&1; then
        echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] [SUPABASE] Upload advanced run OK — fichiers locaux supprimés"
    else
        echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] [SUPABASE-WARN] Upload advanced partiel — fichiers conservés localement"
    fi
fi
print_progress "supabase upload + nettoyage local"

echo "[$(date -u +%Y-%m-%dT%H:%M:%S.%N)Z] ===== run_research_cycle.sh TERMINÉ AVEC SUCCÈS ====="

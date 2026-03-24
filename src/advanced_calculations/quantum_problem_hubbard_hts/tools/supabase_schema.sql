-- SCHEMA SUPABASE — C59/C60 — À exécuter UNE FOIS dans l'éditeur SQL de Supabase
-- URL: https://mwdeqpfxbcdayaelwqht.supabase.co → SQL Editor

CREATE TABLE IF NOT EXISTS quantum_run_files (
    id              BIGSERIAL PRIMARY KEY,
    run_id          TEXT NOT NULL,
    file_path       TEXT NOT NULL,
    file_type       TEXT NOT NULL,
    file_size_bytes BIGINT,
    sha256          TEXT,
    content_text    TEXT,
    uploaded_at     TIMESTAMPTZ DEFAULT NOW(),
    UNIQUE(run_id, file_path)
);
CREATE INDEX IF NOT EXISTS idx_qrf_run_id ON quantum_run_files(run_id);

CREATE TABLE IF NOT EXISTS quantum_csv_rows (
    id          BIGSERIAL PRIMARY KEY,
    run_id      TEXT NOT NULL,
    file_path   TEXT NOT NULL,
    row_index   BIGINT,
    row_json    JSONB,
    uploaded_at TIMESTAMPTZ DEFAULT NOW()
);
CREATE INDEX IF NOT EXISTS idx_qcr_run_id ON quantum_csv_rows(run_id);
CREATE INDEX IF NOT EXISTS idx_qcr_file   ON quantum_csv_rows(run_id, file_path);

-- Activer RLS (Row Level Security) pour sécurité
ALTER TABLE quantum_run_files ENABLE ROW LEVEL SECURITY;
ALTER TABLE quantum_csv_rows  ENABLE ROW LEVEL SECURITY;

-- Policy : service_role peut tout faire (pas de restriction)
CREATE POLICY "service_role full access files" ON quantum_run_files
    FOR ALL USING (true) WITH CHECK (true);
CREATE POLICY "service_role full access rows" ON quantum_csv_rows
    FOR ALL USING (true) WITH CHECK (true);

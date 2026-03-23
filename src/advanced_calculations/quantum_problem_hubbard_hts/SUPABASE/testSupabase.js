python - << 'EOF'
import os
import pg8000

conn = pg8000.connect(
    host=os.getenv("SUPABASE_HOST"),
    port=5432,
    database=os.getenv("SUPABASE_DB"),
    user=os.getenv("SUPABASE_USER"),
    password=os.getenv("SUPABASE_PASSWORD"),
    ssl_context=True
)

cur = conn.cursor()

cur.execute("SELECT COUNT(*) FROM research_modules_config;")
count = cur.fetchone()[0]

print("✅ Connexion OK")
print("📊 Nombre de modules :", count)

cur.execute("SELECT module, lx, ly FROM research_modules_config LIMIT 5;")
rows = cur.fetchall()

print("\n🔍 Aperçu :")
for r in rows:
    print(r)

conn.close()
EOF
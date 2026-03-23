import os
import requests

# Variables d'environnement
DD_API_KEY = os.environ['DD_API_KEY']
DD_SITE = os.environ.get('DD_SITE', 'datadoghq.eu')
DD_ENV = os.environ.get('DD_ENV', 'dev')

logs_url = f"https://http-intake.logs.{DD_SITE}/v1/input/{DD_API_KEY}"

def send_log(message):
    payload = {
        "message": message,
        "ddsource": "replit",
        "service": "lumvorax",
        "env": DD_ENV,
        "hostname": "replit-workspace"
    }
    r = requests.post(logs_url, json=payload)
    if r.status_code == 200:
        print("Log envoyé avec succès: 200")
    else:
        print(f"Erreur lors de l'envoi du log: {r.status_code} {r.text}")

# Exemple
send_log("Test log Replit sans ddtrace natif")
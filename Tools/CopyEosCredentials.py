"""Copia as credenciais da Epic (EOS) do Godot para o Vortex, sem mostrar nada na tela.

python Tools/CopyEosCredentials.py

Origem (somente leitura): the-world-geneses/Systems/Network/eos_credentials.json, as mesmas do
multiplayer do Godot (produto, sandbox, deployment e o cliente). Destino:
Data/Online/eos_credentials.json, que está no .gitignore: o segredo do cliente nunca vai para o git.
O jogo lê esse arquivo quando o jogador abre ou entra numa sala online (VortexOnline).
"""
import json
import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE = ROOT.parent.parent / "the-world-geneses" / "Systems" / "Network" / "eos_credentials.json"
TARGET = ROOT / "Data" / "Online" / "eos_credentials.json"
KEYS = ("product_name", "product_version", "product_id", "sandbox_id", "deployment_id", "client_id", "client_secret")

data = json.loads(SOURCE.read_text(encoding="utf-8"))
missing = [k for k in KEYS if not str(data.get(k, "")).strip()]
TARGET.parent.mkdir(parents=True, exist_ok=True)
TARGET.write_text(json.dumps({k: data.get(k, "") for k in KEYS}, indent=1) + "\n", encoding="utf-8")
# só diz se deu certo; nenhum valor aparece
print("EOS_CREDENTIALS copiadas" + (" (faltando: %s)" % ", ".join(missing) if missing else ""))

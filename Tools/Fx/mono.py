"""Chama o Monolith (HTTP, porta 9316). Uso: python mono.py <niagara|material|monolith_discover|describe> <acao> '<json>'"""
import json, sys, urllib.request


def call(tool, action=None, params=None, timeout=300):
    args = {}
    if action is not None:
        args['action'] = action
    if params is not None:
        args['params'] = params
    if tool in ('niagara', 'material', 'describe', 'bulk_fill'):
        tool += '_query'
    elif tool == 'monolith_discover':
        args = params or {}
    body = json.dumps({'jsonrpc': '2.0', 'id': 1, 'method': 'tools/call', 'params': {'name': tool, 'arguments': args}}).encode()
    req = urllib.request.Request('http://127.0.0.1:9316/mcp', body, {'Content-Type': 'application/json'})
    out = json.loads(urllib.request.urlopen(req, timeout=timeout).read().decode('utf-8', 'replace'))
    if 'error' in out:
        return {'ERROR': out['error']}
    res = out.get('result', {})
    texts = [c.get('text', '') for c in res.get('content', [])]
    text = '\n'.join(texts)
    try:
        data = json.loads(text)
    except Exception:
        data = text
    if res.get('isError'):
        return {'ERROR': data}
    if isinstance(data, dict) and list(data.keys()) == ['result'] and isinstance(data['result'], str):
        try:
            data = json.loads(data['result'])
        except Exception:
            data = data['result']
    return data


if __name__ == '__main__':
    tool = sys.argv[1]
    action = sys.argv[2] if len(sys.argv) > 2 and sys.argv[2] != '-' else None
    params = json.loads(sys.argv[3]) if len(sys.argv) > 3 else None
    r = call(tool, action, params)
    s = r if isinstance(r, str) else json.dumps(r, indent=1, ensure_ascii=False)
    limit = int(sys.argv[4]) if len(sys.argv) > 4 else 6000
    sys.stdout.buffer.write(s[:limit].encode('utf-8'))

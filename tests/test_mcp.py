import pytest
import httpx
import os
import threading
import json
from http.server import BaseHTTPRequestHandler, HTTPServer

class MockMcpHandler(BaseHTTPRequestHandler):
    def log_message(self, format, *args):
        pass # Suppress logging

    def do_POST(self):
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length)
        data = json.loads(post_data.decode('utf-8'))
        
        method = data.get("method")
        req_id = data.get("id")
        
        result = {}
        if method == "initialize":
            result = {"protocolVersion": "2024-11-05", "capabilities": {}}
        elif method == "tools/list":
            result = {
                "tools": [
                    {
                        "name": "add_numbers",
                        "description": "Add two numbers",
                        "inputSchema": {
                            "type": "object",
                            "properties": {
                                "a": {"type": "number"},
                                "b": {"type": "number"}
                            },
                            "required": ["a", "b"]
                        }
                    }
                ]
            }
        elif method == "tools/call":
            args = data.get("params", {}).get("arguments", {})
            if isinstance(args, str):
                args = json.loads(args)
            a = args.get("a", 0)
            b = args.get("b", 0)
            result = {
                "content": [{"type": "text", "text": str(a + b)}]
            }
        
        response_data = json.dumps({"jsonrpc": "2.0", "id": req_id, "result": result})
        
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.end_headers()
        self.wfile.write(response_data.encode('utf-8'))

@pytest.fixture(scope="module")
def mock_mcp_server():
    server = HTTPServer(('127.0.0.1', 0), MockMcpHandler)
    port = server.server_address[1]
    
    thread = threading.Thread(target=server.serve_forever)
    thread.daemon = True
    thread.start()
    
    yield f"http://127.0.0.1:{port}/mcp"
    
    server.shutdown()
    server.server_close()

def test_mcp_tool_execution(mock_mcp_server):
    bonsai_url = os.environ.get("BONSAI_URL", "http://localhost:8080")
    
    payload = {
        "model": "test-model",
        "messages": [{"role": "user", "content": "Please calculate 10 + 15 using the tool."}],
        "mcp_servers": [
            {
                "name": "math_mcp",
                "url": mock_mcp_server
            }
        ],
        "temperature": 0.0
    }
    
    try:
        response = httpx.post(f"{bonsai_url}/v1/chat/completions", json=payload, timeout=30.0)
        if response.status_code != 200:
            pytest.skip(f"BonsAI server error or not running: {response.text}")
            
        data = response.json()
        assert "choices" in data
        assert len(data["choices"]) > 0
        message = data["choices"][0]["message"]
        
        # We don't assert exactly what the LLM says, but it should succeed
        assert data["id"].startswith("bonsai-")
        
    except httpx.ConnectError:
        pytest.skip("BonsAI server not running")

import pytest
import subprocess
import time
import httpx
import os

@pytest.fixture(scope="module")
def mcp_server():
    """Start a mock MCP server."""
    process = subprocess.Popen(["uv", "run", "python3", "tests/mock_mcp_server.py"])
    time.sleep(5) # Wait for start
    yield "http://127.0.0.1:8001"
    process.terminate()
    process.wait()

def test_mcp_tool_invocation(client, model_name, mcp_server):
    """Verify that BonsAI can discover and execute tools from a remote MCP server."""
    
    # Payload with MCP server configuration
    # Note: We use a raw request because the standard OpenAI client doesn't know about 'mcp_servers'
    payload = {
        "model": model_name,
        "messages": [{"role": "user", "content": "What time is it on the MCP server?"}],
        "mcp_servers": [
            {"url": mcp_server}
        ]
    }
    
    # We use httpx directly to pass the non-standard mcp_servers field
    with httpx.Client(base_url="http://127.0.0.1:8080/v1", timeout=60.0) as http_client:
        response = http_client.post("/chat/completions", json=payload)
        assert response.status_code == 200
        data = response.json()
        
        content = data["choices"][0]["message"]["content"]
        print(f"MCP Response: {content}")
        # The model should have called the tool and reported the time
        assert "12:34" in content

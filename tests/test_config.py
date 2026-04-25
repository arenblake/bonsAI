import pytest
import httpx
import subprocess
import time
import os

def test_custom_port(model_name):
    """Verify that the server can bind to and respond on a custom port."""
    custom_port = 9091
    
    # 1. Start the server on a custom port
    cmd = [
        "./bonsai", 
        model_name, 
        "--host", "127.0.0.1", 
        "--port", str(custom_port)
    ]
    
    env = os.environ.copy()
    process = subprocess.Popen(cmd, env=env)
    
    try:
        # 2. Wait for server to initialize
        time.sleep(30)
        
        # 3. Try to list models on the custom port
        with httpx.Client(base_url=f"http://127.0.0.1:{custom_port}/v1") as client:
            response = client.get("/models")
            assert response.status_code == 200
            data = response.json()
            assert "data" in data
            
    finally:
        # 4. Cleanup
        process.terminate()
        process.wait()

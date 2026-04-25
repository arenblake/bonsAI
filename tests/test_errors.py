import pytest
import httpx

def test_missing_messages(raw_client):
    """Verify 400 error when messages are missing."""
    payload = {"model": "some-model"}
    response = raw_client.post("/chat/completions", json=payload)
    # Oat++ currently returns 400 for bad DTO mapping
    assert response.status_code == 400
    assert "error" in response.json()

def test_invalid_role(raw_client):
    """Verify handling of invalid message roles."""
    payload = {
        "model": "gemma",
        "messages": [{"role": "invalid_role", "content": "hello"}]
    }
    response = raw_client.post("/chat/completions", json=payload)
    # The server should handle this gracefully (either ignore or return 400)
    assert response.status_code in [200, 400]

def test_malformed_json(raw_client):
    """Verify 400 error for malformed JSON."""
    headers = {"Content-Type": "application/json"}
    content = '{"messages": [{"role": "user", "content": "hi"}' # Missing closing brace
    response = raw_client.post("/chat/completions", content=content, headers=headers)
    assert response.status_code == 400

def test_invalid_temperature(raw_client):
    """Verify handling of extreme temperature values."""
    payload = {
        "model": "gemma",
        "messages": [{"role": "user", "content": "hi"}],
        "temperature": 100.0 # Very high
    }
    response = raw_client.post("/chat/completions", json=payload)
    # Model should still respond or return error, but not crash
    assert response.status_code in [200, 400]

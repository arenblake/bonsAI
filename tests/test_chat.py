import pytest

def test_chat_non_streaming(client, model_name):
    """Verify basic chat completion without streaming."""
    response = client.chat.completions.create(
        model=model_name,
        messages=[{"role": "user", "content": "What is the capital of France?"}],
        stream=False
    )
    assert len(response.choices) > 0
    assert "Paris" in response.choices[0].message.content
    assert response.model == model_name

def test_chat_streaming(client, model_name):
    """Verify chat completion with SSE streaming."""
    stream = client.chat.completions.create(
        model=model_name,
        messages=[{"role": "user", "content": "Count from 1 to 3."}],
        stream=True
    )
    full_content = ""
    for chunk in stream:
        if chunk.choices[0].delta.content:
            full_content += chunk.choices[0].delta.content
    
    assert "1" in full_content
    assert "3" in full_content

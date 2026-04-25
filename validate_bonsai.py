import openai
import os
import sys

client = openai.OpenAI(
    base_url="http://localhost:8080/v1",
    api_key="sk-no-key-required"
)

def test_models():
    print("--- Testing /v1/models ---")
    try:
        models = client.models.list()
        for model in models.data:
            print(f"Model ID: {model.id}")
    except Exception as e:
        print(f"Error listing models: {e}")

def test_chat_non_streaming():
    print("\n--- Testing /v1/chat/completions (Non-Streaming) ---")
    try:
        response = client.chat.completions.create(
            model="gemma-4-E2B-it.litertlm",
            messages=[{"role": "user", "content": "Tell me a short joke."}],
            stream=False
        )
        print(f"Assistant: {response.choices[0].message.content}")
    except Exception as e:
        print(f"Error in non-streaming completion: {e}")

def test_chat_streaming():
    print("\n--- Testing /v1/chat/completions (Streaming) ---")
    try:
        stream = client.chat.completions.create(
            model="gemma-4-E2B-it.litertlm",
            messages=[{"role": "user", "content": "Count from 1 to 5."}],
            stream=True
        )
        print("Assistant: ", end="", flush=True)
        for chunk in stream:
            if chunk.choices[0].delta.content:
                print(chunk.choices[0].delta.content, end="", flush=True)
        print("\n[Stream Completed]")
    except Exception as e:
        print(f"Error in streaming completion: {e}")

if __name__ == "__main__":
    test_models()
    test_chat_non_streaming()
    test_chat_streaming()

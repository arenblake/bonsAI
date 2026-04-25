import openai
import os
import sys
import json

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

def test_tool_calling():
    print("\n--- Testing /v1/chat/completions (Tool Calling) ---")
    tools = [
        {
            "type": "function",
            "function": {
                "name": "get_current_weather",
                "description": "Get the current weather in a given location",
                "parameters": {
                    "type": "object",
                    "properties": {
                        "location": {
                            "type": "string",
                            "description": "The city and state, e.g. San Francisco, CA",
                        },
                        "unit": {"type": "string", "enum": ["celsius", "fahrenheit"]},
                    },
                    "required": ["location"],
                },
            },
        }
    ]
    
    messages = [{"role": "user", "content": "What's the weather like in Boston?"}]
    
    try:
        print("Requesting tool call...")
        response = client.chat.completions.create(
            model="gemma-4-E2B-it.litertlm",
            messages=messages,
            tools=tools,
            tool_choice="auto"
        )
        
        message = response.choices[0].message
        if message.tool_calls:
            for tool_call in message.tool_calls:
                print(f"Tool Call Found: {tool_call.function.name}({tool_call.function.arguments})")
                
                # Simulate tool response
                messages.append(message)
                messages.append({
                    "role": "tool",
                    "tool_call_id": tool_call.id,
                    "content": json.dumps({"temperature": 22, "unit": "celsius", "description": "Partly cloudy"})
                })
                
                print("Sending tool response back to model...")
                final_response = client.chat.completions.create(
                    model="gemma-4-E2B-it.litertlm",
                    messages=messages
                )
                print(f"Assistant Final Answer: {final_response.choices[0].message.content}")
        else:
            print(f"Assistant (No tool call): {message.content}")
            
    except Exception as e:
        print(f"Error in tool calling test: {e}")

import time

if __name__ == "__main__":
    test_models()
    time.sleep(2)
    test_chat_non_streaming()
    time.sleep(2)
    test_chat_streaming()
    time.sleep(2)
    test_tool_calling()

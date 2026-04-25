import pytest
import json

def test_tool_calling_loop(client, model_name):
    """Verify the full tool calling loop."""
    tools = [
        {
            "type": "function",
            "function": {
                "name": "get_stock_price",
                "description": "Get the stock price for a given symbol",
                "parameters": {
                    "type": "object",
                    "properties": {
                        "symbol": {"type": "string", "description": "Ticker symbol"}
                    },
                    "required": ["symbol"]
                }
            }
        }
    ]
    
    messages = [{"role": "user", "content": "What is the stock price of AAPL?"}]
    
    # 1. Model requests tool call
    response = client.chat.completions.create(
        model=model_name,
        messages=messages,
        tools=tools
    )
    
    message = response.choices[0].message
    assert message.tool_calls is not None
    assert message.tool_calls[0].function.name == "get_stock_price"
    
    # 2. Provide tool response
    messages.append(message)
    messages.append({
        "role": "tool",
        "tool_call_id": message.tool_calls[0].id,
        "content": json.dumps({"price": 175.50, "currency": "USD"})
    })
    
    # 3. Model gives final answer
    final_response = client.chat.completions.create(
        model=model_name,
        messages=messages
    )
    
    assert len(final_response.choices[0].message.content) > 0
    assert "175.50" in final_response.choices[0].message.content

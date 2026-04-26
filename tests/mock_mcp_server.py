from fastapi import FastAPI, Request
import uvicorn
import json

app = FastAPI()

@app.post("/")
async def mcp_handler(request: Request):
    body = await request.json()
    method = body.get("method")
    rpc_id = body.get("id")
    
    print(f"[Mock MCP] Method: {method}, ID: {rpc_id}")
    
    result = {}
    if method == "initialize":
        result = {
            "protocolVersion": "2024-11-05",
            "capabilities": {
                "tools": {"listChanged": False}
            },
            "serverInfo": {
                "name": "MockMCPServer",
                "version": "1.0.0"
            }
        }
    elif method == "tools/list":
        result = {
            "tools": [
                {
                    "name": "mcp_get_time",
                    "description": "Get the current time from MCP server",
                    "inputSchema": {
                        "type": "object",
                        "properties": {},
                        "required": []
                    }
                }
            ]
        }
    elif method == "tools/call":
        params = body.get("params", {})
        name = params.get("name")
        print(f"[Mock MCP] Calling tool {name} with args: {params.get('arguments')}")
        if name == "mcp_get_time":
            result = {
                "content": [
                    {
                        "type": "text",
                        "text": "The current time on the MCP server is 12:34 PM UTC."
                    }
                ],
                "isError": False
            }
        else:
            return {
                "jsonrpc": "2.0",
                "id": rpc_id,
                "error": {"code": -32601, "message": "Method not found"}
            }
    else:
        return {
            "jsonrpc": "2.0",
            "id": rpc_id,
            "error": {"code": -32601, "message": "Method not found"}
        }

    return {
        "jsonrpc": "2.0",
        "id": rpc_id,
        "result": result
    }

if __name__ == "__main__":
    uvicorn.run(app, host="127.0.0.1", port=8001)

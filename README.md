# BonsAI 🪴

BonsAI is a high-performance, edge-optimized inference server that provides an OpenAI-compatible REST API for models running on the **LiteRT-LM** (Google AI Edge) backend.

## 🚀 Features

- **OpenAI Compatible**: Support for `/v1/chat/completions` and `/v1/models`.
- **Streaming Support**: Real-time token streaming via Server-Sent Events (SSE).
- **Edge Optimized**: Built with C++20 and LiteRT-LM for minimal overhead.
- **Hardware Acceleration**: Seamlessly leverages CPU/GPU/NPU via LiteRT backends.

## 🛠️ Setup

### Prerequisites

- Ubuntu 24.04 (or similar Linux)
- CMake 3.25+
- Bazel (for building the LiteRT-LM shared library)
- C++20 Compiler (GCC 13+ or Clang 18+)

### 1. Build the Engine

BonsAI uses a custom-built shared library of LiteRT-LM to ensure portability.

```bash
cd LiteRT-LM
bazel build -c opt //c:libbonsai_engine.so
cd ..
```

### 2. Build the Server

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## 🏃 Usage

Start the server by pointing it to a `.litertlm` model file:

```bash
export LD_LIBRARY_PATH=$PWD/LiteRT-LM/bazel-bin/c:$LD_LIBRARY_PATH
./build/bonsai path/to/your_model.litertlm
```

The server will listen on `http://0.0.0.0:8080`.

### Example Request (cURL)

```bash
curl http://localhost:8080/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{
    "model": "gemma-4-E2B-it",
    "messages": [{"role": "user", "content": "What is BonsAI?"}],
    "stream": true
  }'
```

### Example Usage (Python Client)

```python
import openai

client = openai.OpenAI(base_url="http://localhost:8080/v1", api_key="sk-none")

response = client.chat.completions.create(
    model="gemma-4-E2B-it",
    messages=[{"role": "user", "content": "Tell me a story about a small tree."}],
    stream=True
)

for chunk in response:
    print(chunk.choices[0].delta.content or "", end="")
```

## 📋 Roadmap

- [x] OpenAI-standard Chat API
- [x] SSE Streaming
- [x] Multi-client thread-safe orchestration
- [x] Advanced Tool/Function Calling
- [ ] Multimodal (Vision/Audio) input support
- [ ] Static binary distribution

## 📄 License

Apache 2.0 - See `LICENSE` for details.

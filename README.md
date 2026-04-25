# BonsAI 🪴

BonsAI is a high-performance, edge-optimized inference server that provides an OpenAI-compatible REST API for models running on the **LiteRT-LM** (Google AI Edge) backend.

It is designed to be a lightweight, drop-in replacement for the `llama.cpp` server, specifically tailored for resource-constrained devices like Raspberry Pi, NVIDIA Jetson, and other edge hardware.

## 🚀 Features

- **OpenAI Compatible**: Support for `/v1/chat/completions` and `/v1/models`.
- **Streaming Support**: Real-time token streaming via Server-Sent Events (SSE).
- **Edge Optimized**: Built with C++20 and LiteRT-LM for minimal overhead.
- **Hardware Acceleration**: Seamlessly leverages CPU/GPU/NPU via LiteRT backends.
- **Multimodal Support**: Interleaved Text, Image (Vision), and Audio input support.
- **Zero-Dependency Binary**: Distribute as a single static executable.

## 🛠️ Setup

### Prerequisites

- **OS**: Ubuntu 24.04 (or similar Linux)
- **Build Tools**: Bazel (7.0+), C++20 Compiler (GCC 13+ or Clang 18+)
- **Libraries**: [Oat++](https://oatpp.io/) installed at `/usr/local`

### 1. Clone the Repository
```bash
git clone --recursive https://github.com/arenblake/bonsAI.git
cd bonsAI
```

### 2. Configure the Workspace
The configuration script sets up the necessary symlinks between the BonsAI source and the LiteRT-LM Bazel workspace.
```bash
chmod +x configure.sh
./configure.sh
```

### 3. Build the Monolithic Binary
BonsAI is compiled into a single self-contained binary using Bazel.
```bash
cd LiteRT-LM
bazel build -c opt //bonsai:bonsai
cd ..
cp LiteRT-LM/bazel-bin/bonsai/bonsai .
```

## 🏃 Usage

Start the server by pointing it to a `.litertlm` model file:

```bash
./bonsai path/to/your_model.litertlm
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

## 🧪 Testing

BonsAI uses `pytest` for automated validation.

### Prerequisites for Testing
- `uv` (recommended for Python dependency management)
- `ffmpeg` (required for multimodal audio tests)

### Running the Test Suite
1. **Start the server** (Terminal 1):
   ```bash
   ./bonsai path/to/gemma-4-E2B-it.litertlm
   ```
2. **Run tests** (Terminal 2):
   ```bash
   # Install dependencies
   uv pip install pytest pytest-asyncio openai httpx

   # Run the suite
   uv run pytest tests/
   ```

## 📋 Roadmap

- [x] OpenAI-standard Chat API
- [x] SSE Streaming
- [x] Multi-client thread-safe orchestration
- [x] Advanced Tool/Function Calling
- [x] Multimodal (Vision/Audio) input support
- [x] Static binary distribution
- [ ] Static binary distribution via Docker (for `manylinux` compatibility)
- [ ] CUDA/OpenCL Backend support in binary releases

## 📄 License

Apache 2.0 - See `LICENSE` for details.

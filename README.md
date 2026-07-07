# Ultron

A fully offline AI assistant for Android powered by **llama.cpp**.

## Before You Build

### 1. Clone with the llama.cpp submodule

```bash
git clone --recursive https://github.com/arkofeloah/Ultron.git
```

If you already cloned the repository:

```bash
git submodule update --init --recursive
```

Without this, the project will not compile because `native/llama.cpp` will be empty.

---

### 2. Android Studio Requirements

Install:

- Android SDK
- Android NDK
- CMake

Android Studio will usually prompt you to install any missing components.

---

### 3. GGUF Models

Ultron only supports **GGUF** models.

A GGUF file contains the AI model in a format that `llama.cpp` can run.

Example:

```
Qwen2.5-0.5B-Instruct-Q4_K_M.gguf
```

The parts of the filename mean:

- **Qwen2.5** → Model family
- **0.5B** → 500 million parameters
- **Instruct** → Fine-tuned for chatting
- **Q4_K_M** → 4-bit quantization (smaller and faster)
- **.gguf** → Model file format

---

### 4. Downloading Models

Download GGUF models from Hugging Face.

Make sure you download the **.gguf** file, **not** the PyTorch (`.bin`) or Safetensors (`.safetensors`) versions.

---

### 5. Where to Place the Model

Place the downloaded `.gguf` file in:

```
Internal Storage/
└── Download/
```

Ultron currently loads models from this location.

---

### 6. First Build

Open the project in Android Studio and click **Run**.

The first build may take several minutes because Gradle and the NDK need to download dependencies.

---

## Licensing

Ultron is source-available.

The source code is publicly visible for learning and research.

Commercial use, redistribution, or integration into another product requires prior written permission from the author.

If you are interested in licensing Ultron for commercial use, please contact


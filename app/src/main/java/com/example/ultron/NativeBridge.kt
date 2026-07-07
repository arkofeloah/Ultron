package com.example.ultron

object NativeBridge {
    init {
        System.loadLibrary("ultron")
    }

    interface GenerationListener {
        fun onToken(piece: String)
        fun onTraceUpdate(genCount: Int, elapsedMs: Long, tps: Float)
    }

    external fun helloFromNative(): String

    external fun initializeLlamaBackend(): String

    external fun checkModelFile(modelPath: String): String

    external fun loadModel(modelPath: String): String

    external fun generate(prompt: String, listener: GenerationListener): String
}

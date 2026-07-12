package com.example.ultron

import android.os.Bundle
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.text.selection.SelectionContainer
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.example.ultron.ui.theme.UltronTheme
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File
import java.io.FileOutputStream

data class GenerationTrace(
    val genCount: Int = 0,
    val elapsedMs: Long = 0,
    val tps: Float = 0f
)

class MainActivity : ComponentActivity() {
    private val modelName = "qwen2.5-0.5b-instruct-q4_k_m.gguf"

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        Log.d("ULTRON", NativeBridge.helloFromNative())
        Log.d("ULTRON", NativeBridge.initializeLlamaBackend())

        val modelsDir = File(filesDir, "models")
        if (!modelsDir.exists()) modelsDir.mkdirs()
        
        val modelFile = File(modelsDir, modelName)
        if (!modelFile.exists()) {
            copyAssetToInternalStorage(modelName, modelFile)
        }
        
        val absolutePath = modelFile.absolutePath
        Log.d("ULTRON", "Target path: $absolutePath")

        val fileStatus = NativeBridge.checkModelFile(absolutePath)
        Log.d("ULTRON", "Model file status: $fileStatus")

        var modelLoaded = false
        if (fileStatus == "FILE FOUND") {
            val loadStatus = NativeBridge.loadModel(absolutePath)
            Log.d("ULTRON", "Load result: $loadStatus")
            modelLoaded = (loadStatus == "MODEL LOADED")
        }

        enableEdgeToEdge()
        setContent {
            UltronTheme {
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    UltronUI(
                        modelLoaded = modelLoaded,
                        modifier = Modifier.padding(innerPadding)
                    )
                }
            }
        }
    }

    private fun copyAssetToInternalStorage(assetName: String, targetFile: File) {
        try {
            assets.open("models/$assetName").use { inputStream ->
                FileOutputStream(targetFile).use { outputStream ->
                    val buffer = ByteArray(1024 * 8)
                    var read: Int
                    while (inputStream.read(buffer).also { read = it } != -1) {
                        outputStream.write(buffer, 0, read)
                    }
                }
            }
            Log.d("ULTRON", "Model copied to internal storage: ${targetFile.absolutePath}")
        } catch (e: Exception) {
            Log.e("ULTRON", "Failed to copy asset model", e)
        }
    }
}

@Composable
fun UltronUI(modelLoaded: Boolean, modifier: Modifier = Modifier) {
    var prompt by remember { mutableStateOf("") }
    var response by remember { mutableStateOf("") }
    var isGenerating by remember { mutableStateOf(false) }
    var trace by remember { mutableStateOf(GenerationTrace()) }
    val scope = rememberCoroutineScope()

    Column(
        modifier = modifier
            .fillMaxSize()
            .padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        Text(
            text = "Model Status: ${if (modelLoaded) "Loaded" else "Not Loaded"}",
            style = MaterialTheme.typography.labelSmall
        )

        TextField(
            value = prompt,
            onValueChange = { prompt = it },
            label = { Text("Enter prompt") },
            modifier = Modifier.fillMaxWidth(),
            enabled = !isGenerating && modelLoaded
        )

        Button(
            onClick = {
                if (prompt.isNotBlank() && !isGenerating) {
                    isGenerating = true
                    response = ""
                    trace = GenerationTrace()
                    scope.launch(Dispatchers.Default) {
                        try {
                            val listener = object : NativeBridge.GenerationListener {
                                override fun onToken(piece: String) {
                                    scope.launch(Dispatchers.Main) {
                                        response += piece
                                    }
                                }

                                override fun onTraceUpdate(genCount: Int, elapsedMs: Long, tps: Float) {
                                    scope.launch(Dispatchers.Main) {
                                        trace = GenerationTrace(genCount, elapsedMs, tps)
                                    }
                                }
                            }
                            val finalResult = NativeBridge.generate(prompt, listener)
                            withContext(Dispatchers.Main) {
                                // Final sync in case some pieces were missed or final formatting is needed
                                Log.d("ULTRON", "Generation finished: $finalResult")
                            }
                        } catch (e: Exception) {
                            withContext(Dispatchers.Main) {
                                response = "ERROR: ${e.message}"
                                Log.e("ULTRON", "Generation failed", e)
                            }
                        } finally {
                            withContext(Dispatchers.Main) {
                                isGenerating = false
                            }
                        }
                    }
                }
            },
            modifier = Modifier.align(Alignment.End),
            enabled = !isGenerating && prompt.isNotBlank() && modelLoaded
        ) {
            if (isGenerating) {
                CircularProgressIndicator(
                    modifier = Modifier.size(16.dp),
                    strokeWidth = 2.dp,
                    color = MaterialTheme.colorScheme.onPrimary
                )
                Spacer(Modifier.width(8.dp))
            }
            Text("Generate")
        }

        HorizontalDivider()

        // Terminal-style Trace Panel
        Box(
            modifier = Modifier
                .fillMaxWidth()
                .height(80.dp)
                .background(Color.Black)
                .padding(8.dp)
        ) {
            Column {
                Text(
                    text = "TRACE: ${trace.genCount} tokens | ${trace.elapsedMs} ms | ${"%.2f".format(trace.tps)} t/s",
                    color = Color.Green,
                    fontFamily = FontFamily.Monospace,
                    fontSize = 12.sp
                )
            }
        }

        Box(
            modifier = Modifier
                .fillMaxSize()
                .verticalScroll(rememberScrollState())
        ) {
            SelectionContainer {
                Text(text = response)
            }
        }
    }
}

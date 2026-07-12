# Ultron

Offline Android AI assistant powered by llama.cpp and GGUF models.

This repository contains the Ultron Android application source code, native llama.cpp integration, and the required Android build configuration.

The project is designed to run fully offline using local GGUF models.

---

# Requirements

## Hardware

Recommended:

* Android phone with:

  * Android 8.0 (API 26) or higher
  * Minimum 2 GB free storage
  * Minimum 2 GB RAM recommended

Development machine:

* Windows / Linux / macOS
* At least 10 GB free storage
* USB cable for device installation

---

# Required Software

## 1. Android Studio

Install the latest Android Studio.

Required components:

* Android SDK
* Android SDK Platform
* Android SDK Build Tools
* Android NDK
* CMake

During installation, make sure these are selected:

```
Android SDK
Android SDK Platform
Android Virtual Device
Android NDK
CMake
```

---

## 2. Java JDK

Ultron requires Java 17 or newer.

Android Studio includes a compatible JDK.

Recommended:

```
Android Studio Embedded JDK
```

Example location:

```
C:\Program Files\Android\Android Studio\jbr
```

Verify:

```powershell
java -version
```

---

## 3. Git

Install Git:

https://git-scm.com/

Verify:

```powershell
git --version
```

---

# Clone the Repository

Open PowerShell or Terminal.

Navigate to your Android projects folder:

Example:

```powershell
cd C:\Users\YOUR_USERNAME\AndroidStudioProjects
```

Clone the repository with submodules:

```powershell
git clone --recursive https://github.com/arkofeloah/Ultron.git
```

Enter the project:

```powershell
cd Ultron
```

---

# Initialize llama.cpp Submodule

Ultron uses llama.cpp as a Git submodule.

If you did not use `--recursive` during cloning:

Run:

```powershell
git submodule update --init --recursive
```

Verify:

```powershell
git submodule status
```

Expected output should show:

```
native/llama.cpp
```

---

# Open Project in Android Studio

1. Open Android Studio.
2. Select:

```
Open
```

3. Choose:

```
Ultron
```

4. Wait for Gradle Sync to finish.

Do not manually modify Gradle versions unless required.

---

# Configure Android SDK

Open:

```
File
 → Settings
 → Languages & Frameworks
 → Android SDK
```

Confirm:

Installed SDK Platforms:

```
Android API 36
```

Installed SDK Tools:

```
Android SDK Build Tools
Android SDK Platform Tools
NDK
CMake
```

---

# Add GGUF Model

Ultron uses GGUF models.

The model is not stored in GitHub.

Download a compatible GGUF model.

Example:

```
qwen2.5-0.5b-instruct-q4_k_m.gguf
```

Place it here:

```
app/
 └── src/
     └── main/
         └── assets/
             └── models/
                 └── qwen2.5-0.5b-instruct-q4_k_m.gguf
```

The folder structure must exactly match:

```
app/src/main/assets/models/
```

---

# Build the Application

From the project root:

Windows:

```powershell
.\gradlew assembleDebug
```

Linux/macOS:

```bash
./gradlew assembleDebug
```

Successful build:

```
BUILD SUCCESSFUL
```

The APK will be generated at:

```
app/build/outputs/apk/debug/app-debug.apk
```

---

# Install APK on Phone

Enable:

```
Developer Options
USB Debugging
```

Connect the phone.

Verify:

```powershell
adb devices
```

Expected:

```
List of devices attached

DEVICE_ID    device
```

Install:

```powershell
adb install app/build/outputs/apk/debug/app-debug.apk
```

---

# First Launch

On first launch:

1. Grant required permissions.
2. Wait for model initialization.
3. The application will load the GGUF model.

The first startup may take longer because the model needs to initialize.

---

# Troubleshooting

## Gradle cannot find Java

Error:

```
JAVA_HOME is not set
```

Set:

Windows PowerShell:

```powershell
setx JAVA_HOME "C:\Program Files\Android\Android Studio\jbr"
```

Restart PowerShell.

Verify:

```powershell
echo $env:JAVA_HOME
```

---

## llama.cpp submodule missing

Run:

```powershell
git submodule update --init --recursive
```

---

## Model not detected

Check:

```
app/src/main/assets/models/
```

Confirm the `.gguf` file exists.

Correct:

```
app/src/main/assets/models/model.gguf
```

Incorrect:

```
app/src/main/assets/model.gguf
```

---

## Clean build

If Gradle problems occur:

```powershell
.\gradlew clean
```

Then:

```powershell
.\gradlew assembleDebug
```

---

# Repository Structure

```
Ultron
│
├── app
│   └── Android application
│
├── native
│   └── llama.cpp
│
├── gradle
│
├── build.gradle.kts
│
└── settings.gradle.kts
```

---

# Important Notes

* GGUF models are intentionally not stored in GitHub.
* Add your required model manually before building.
* Do not commit APK files or model files.
* Always clone with submodules enabled.

---

# Quick Setup Summary

For a fresh machine:

```powershell
git clone --recursive https://github.com/arkofeloah/Ultron.git

cd Ultron

git submodule update --init --recursive

# Add GGUF model to:
# app/src/main/assets/models/

.\gradlew assembleDebug
```

The generated APK can then be installed on an Android device.

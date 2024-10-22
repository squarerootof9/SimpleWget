# SimpleWget
# Simple Wget for Windows

A simple command-line utility for downloading files over HTTP and HTTPS on Windows, similar to the Unix `wget` tool.

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Compilation](#compilation)
  - [Prerequisites](#prerequisites)
  - [Compilation Instructions](#compilation-instructions)
- [Usage](#usage)
  - [Basic Usage](#basic-usage)
  - [Command-Line Options](#command-line-options)
- [Examples](#examples)
- [License](#license)
- [Acknowledgments](#acknowledgments)

---

## Overview

This project provides a simple implementation of `wget` for Windows. It allows users to download files from the internet via the command line, supporting basic features such as HTTP/HTTPS downloads, authentication, SSL/TLS settings, and more.

---

## Features

- Supports HTTP and HTTPS protocols
- Basic authentication support
- SSL/TLS protocol selection
- Option to ignore SSL certificate errors
- Customizable number of retries
- Option to prevent overwriting existing files (`--no-clobber`)
- Server response headers display
- Debugging information output

---

## Compilation

### Prerequisites

To compile the `s_wget` application, you need:

- **Microsoft Visual Studio Build Tools**: You can download them from [Microsoft's official website](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022). Make sure to install the "Desktop development with C++" workload.

### Compilation Instructions

1. **Download the Source Code**: Save the `s_wget.cpp` file from this repository to your local machine.

2. **Open the Developer Command Prompt**:

   - Navigate to the **Start Menu** > **Visual Studio 2022** > **x64 Native Tools Command Prompt for VS 2022**.

3. **Navigate to the Source Code Directory**:

   ```cmd
   cd path\to\your\source\code
   ```

4. **Compile the Source Code**:

   Use the following command to compile `s_wget.cpp`:

   ```cmd
   cl /EHsc /W4 s_wget.cpp /link winhttp.lib wininet.lib
   ```

   - **`/EHsc`**: Enables standard exception handling.
   - **`/W4`**: Sets the warning level to 4 (informational warnings).
   - **`/link winhttp.lib wininet.lib`**: Links against the `winhttp` and `wininet` libraries required for HTTP/HTTPS functionality.

5. **Successful Compilation**:

   If the compilation is successful, you will have `s_wget.exe` in your current directory.

---

## Usage

### Basic Usage

The basic syntax for using `s_wget` is:

```cmd
s_wget [options] <URL>
```

### Command-Line Options

- `-V`, `--version`: Display version information and exit.
- `-h`, `--help`: Display help information and exit.
- `-nc`, `--no-clobber`: Skip downloads that would overwrite existing files.
- `-t`, `--tries=NUMBER`: Set the number of retries (0 for unlimited).
- `-O`, `--output-document=FILE`: Write the downloaded document to `FILE`.
- `-S`, `--server-response`: Print server response headers.
- `-d`, `--debug`: Print debugging information.

**HTTP Options**:

- `--http-user=USER`: Set HTTP authentication user.
- `--http-password=PASS`: Set HTTP authentication password.

**HTTPS (SSL/TLS) Options**:

- `--no-check-certificate`: Disable SSL certificate checking.
- `--https-only`: Only follow secure HTTPS links.
- `--secure-protocol=PR`: Choose secure protocol (`auto`, `TLSv1`, `TLSv1_1`, `TLSv1_2`).

---

## Examples

1. **Download a File to the Current Directory**:

   ```cmd
   s_wget.exe https://example.com/file.exe
   ```

2. **Download a File and Save with a Specific Name**:

   ```cmd
   s_wget.exe https://example.com/file.exe -O file.exe
   ```

3. **Download a File Without Overwriting Existing Files**:

   ```cmd
   s_wget.exe --no-clobber https://example.com/file.exe
   ```

4. **Download a File with HTTP Authentication**:

   ```cmd
   s_wget.exe --http-user=username --http-password=secret https://example.com/protected.exe
   ```

5. **Download Ignoring SSL Certificate Errors**:

   ```cmd
   s_wget.exe --no-check-certificate https://example.com/file.exe
   ```

6. **Display Server Response Headers**:

   ```cmd
   s_wget.exe -S https://example.com/file.exe
   ```

---

## License

This project is licensed under the MIT License.

```
MIT License

[Full MIT License Text]
```

---

## Acknowledgments

- **WinHTTP API**: For providing HTTP/HTTPS functionalities.
- **Microsoft Visual Studio**: For the development tools.

---

**Note**: This application is a simplified version of `wget` and may not support all features available in the GNU `wget` utility. Use it at your own risk and ensure to test it in your environment.

---

Let me know if you need any further assistance or modifications!

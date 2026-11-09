# qmanga

**qmanga** is a multithreaded manga, comic, and book reader built with **C++20 and Qt 6** for Linux and Windows. It combines a page viewer, database-backed bookshelf, support for several document formats, and interactive OCR and translation tools for reading Japanese, Chinese, and other text in scanned pages.

Full-page OCR can locate multiple text regions and display their outlines directly on the page. Hover over a recognized region to see its text and, when a translation backend is configured, a translation. Select regions to collect their original text in the OCR editor for review or translation as a longer passage.

> **Development status:** qmanga is a source-built desktop project. Available features depend on your build, installed libraries, configured OCR engines, and translation services. External ML servers are optional.

## Features

- **Multithreaded image viewer** with page navigation, scaling, and image-processing controls.
- **Bookshelf and collection management** backed by SQLite or MySQL-compatible databases.
- **Multiple input formats**, including image files and directories, ZIP/RAR archives, PDF, DjVu, TXT, FB2, and EPUB (where supported).
- **Office-document import** through LibreOffice conversion to PDF, including common Word, spreadsheet, presentation, and OpenDocument formats.
- **Selectable OCR backends:** Tesseract for manually selected areas; Google Cloud Vision or PaddleX for full-page text-region detection and recognition.
- **Interactive full-page OCR overlay:** detected polygons align with the displayed page; hover for recognized text and available translations, or click a text region to append its source text to the OCR editor.
- **Translation backends:** [jpreader](https://github.com/kernel1024/jpreader) over D-Bus on Linux, and [Ollama](https://ollama.com/) through an HTTP endpoint on supported platforms.
- **Remote inference:** use a desktop or LAN machine for PaddleX OCR and Ollama translation while running qmanga on a lighter client.

## Supported platforms

| Platform | Build notes | Translation integration |
| --- | --- | --- |
| **Linux** | Qt 6 and the system development libraries; optional components are detected by qmake | jpreader via D-Bus (when available); Ollama via HTTP |
| **Windows 11** | Native build with **MSYS2 UCRT64**, Qt 6, and compatible UCRT64 dependencies | Ollama via HTTP; jpreader's D-Bus integration is not available |

**WSL is not required** for the native Windows executable. EPUB support depends on the availability of `libepub` and the build configuration; do not assume it is enabled on Windows.

## Supported content

| Category | Formats / mechanism |
| --- | --- |
| Images and comics | Supported image files and image directories, ZIP, RAR |
| Scanned documents | PDF (Poppler), DjVu (DjVuLibre) |
| Text and e-books | TXT, FB2, EPUB (optional `libepub`) |
| Office documents | RTF, DOC, DOCX, XLS, XLSX, PPT, PPTX, WPS, ODT, ODS, ODP via LibreOffice-to-PDF conversion |

Some formats require optional libraries or external applications. Reader availability can differ between builds.

## Requirements

**Core build requirements**

- C++20 compiler and **Qt 6.5+** (`Core`, `Gui`, `Widgets`, `Sql`, `Xml`, `Network`; `DBus` on Linux)
- qmake and `pkg-config`/`pkgconf`
- `libzip` and `zlib`
- ICU (`icu-uc`, `icu-i18n`)
- oneTBB / Intel TBB
- OpenSSL
- Qt SQL drivers for **SQLite** and/or **MySQL**; the corresponding database/client libraries as needed

**Optional components**

- [Poppler](https://poppler.freedesktop.org/) — PDF support (the Linux project configuration checks for `poppler` and `poppler-cpp`)
- [Tesseract OCR](https://github.com/tesseract-ocr/tesseract) with the appropriate language data — selection-based offline OCR
- [DjVuLibre](https://djvu.sourceforge.net/) — DjVu support
- `ebook-tools` / `libepub` — EPUB support, where detected by the build
- [LibreOffice](https://www.libreoffice.org/) — conversion of office documents for viewing
- [Google Cloud Vision](https://cloud.google.com/vision/docs/ocr) credentials — cloud OCR
- [PaddleX](https://github.com/PaddlePaddle/PaddleX) serving installation — self-hosted ML OCR
- [Ollama](https://ollama.com/) with a suitable model — local or LAN translation
- [jpreader](https://github.com/kernel1024/jpreader) — optional Linux D-Bus translation integration

The HTTP-based OCR and translation clients do **not** require PaddlePaddle or an LLM runtime to be installed on the qmanga client itself.

## Building

### Linux

Install the development packages corresponding to the requirements above. Package names vary by distribution. Then:

```bash
git clone https://github.com/kernel1024/qmanga.git
cd qmanga
qmake6 qmanga.pro CONFIG+=release
make -j"$(nproc)"
```

Depending on your distribution, the Qt 6 qmake executable may be named `qmake` rather than `qmake6`. Check the dependency-detection messages printed by qmake to confirm which optional readers and OCR components are enabled.

The project also provides installation rules for the executable, desktop entry, and icon. Use the appropriate installation prefix and permissions for your environment.

### Windows 11 — MSYS2 UCRT64

Use the [MSYS2 UCRT64](https://www.msys2.org/docs/environments/) shell and install the matching **UCRT64** versions of Qt 6, the compiler/toolchain, `pkgconf`, and the libraries listed above. In particular, use compatible UCRT64 builds of Poppler, Tesseract, DjVuLibre, ICU, TBB, libzip, OpenSSL, and any required database drivers.

Build with qmake and MinGW Make:

```bash
git clone https://github.com/kernel1024/qmanga.git
cd qmanga
qmake6 qmanga.pro CONFIG+=release
mingw32-make -j8
```

Use the Qt Creator kit from the same MSYS2 environment if building in an IDE. **Do not mix incompatible MSVC, MSYS2 MINGW64, and UCRT64 libraries.** The Windows project configuration expects several optional document libraries to be installed, so an incomplete dependency set may cause the build to fail.

For distributing a Windows executable, deploy the required Qt plugins and DLLs (for example, using the Qt `windeployqt` tool) and verify the remaining non-Qt dependencies separately. Test the packaged application on a Windows system that does not have your development environment installed.

## OCR

qmanga supports both manual selection and full-page recognition:

| OCR backend | Manually selected area | Entire page with coordinates | Processing location |
| --- | --- | --- | --- |
| **Tesseract** | Yes | No | Local library |
| **Google Cloud Vision** | Yes | Yes | Google Cloud |
| **PaddleX / PaddleOCR** | Yes | Yes | Self-hosted HTTP server |

For full-page recognition, Google Vision and PaddleX return multiple text regions with their coordinates. qmanga draws those regions on the page; hovering shows the recognized text. A configured translation backend can add the translation beneath a separator in the tooltip. Clicking a recognized region sends the **original text** to the OCR editor.

**Note:** an OCR text region is not necessarily an entire speech bubble. A single bubble may contain multiple detected lines/regions, especially with PaddleX. Use the OCR editor to assemble dialogue in the desired reading order when translating a longer passage.

### PaddleX: self-hosted full-page OCR

Useful references: [PaddleX](https://github.com/PaddlePaddle/PaddleX), [PaddleOCR](https://github.com/PaddlePaddle/PaddleOCR), and the [official serving guide](https://www.paddleocr.ai/main/en/version3.x/inference_deployment/serving/serving.html).

PaddleX runs independently of qmanga and can use the CPU or a compatible GPU on another computer. Install and test the OCR pipeline inside your preferred Python/Conda environment, following the PaddleX installation guide. The first run may download model files; ensure they are cached before expecting fully offline operation.

A basic test (with a local image) is:

```bash
paddlex --pipeline OCR --input manga_page.jpg
```

Install the serving extras required by your PaddleX version and run the OCR service:

```bash
paddlex --install serving
paddlex --serve --pipeline OCR --host 127.0.0.1 --port 8080
```

To serve other machines on a trusted LAN, replace `127.0.0.1` with the server's LAN IP and allow the chosen port through its firewall. Configure qmanga's **PaddleX server URL** to point to the `/ocr` endpoint, for example:

```text
http://192.168.88.20:8080/ocr
```

`192.168.88.20` is an example address, not a default. The client sends an image to the PaddleX HTTP API and reads recognized text, confidence scores, and polygons from the response. No PaddlePaddle installation is necessary on the computer running qmanga.

For routine manga reading, the usual cycle is **open page → trigger full-page OCR → hover over text regions → advance to the next page**.

### Google Cloud Vision

Google Vision performs OCR through the Google Cloud API. Configure the credentials and API settings in qmanga and select **Google Vision** as the OCR engine. Full-page recognition returns positioned text regions for the same overlay and tooltip workflow as PaddleX.

See [Google Cloud Vision OCR documentation](https://cloud.google.com/vision/docs/ocr) for setup, authentication, usage limits, and pricing. Unlike PaddleX, Google Vision sends image data to an external cloud service.

## Translation

Translation is separate from OCR. qmanga can display recognized text even when no translation service is configured.

- **Ollama**: HTTP-based translation using a model hosted on the same machine or another computer on the LAN. Useful on both Linux and Windows.
- **jpreader**: optional Linux-only D-Bus integration with the [jpreader project](https://github.com/kernel1024/jpreader), which supports several translation services.

The translation implementation supports asynchronous requests; the OCR viewer can show recognized regions while translations are being generated. The OCR editor can also submit its complete contents as one passage, preserving the order in which you collected text from the page.

### Ollama: self-hosted translation

References: [Ollama](https://ollama.com/), [model library](https://ollama.com/library), [API documentation](https://docs.ollama.com/api/generate), and [Ollama networking FAQ](https://docs.ollama.com/faq).

Install Ollama on the host that will run the model, then pull a translation-capable model. For example, if you want to test TranslateGemma and the corresponding model is available in your Ollama installation:

```bash
ollama pull translategemma:4b
```

Ollama normally exposes an HTTP API on localhost, commonly port `11434`. On a Linux host, you can start a server bound to a chosen LAN interface with a command like:

```bash
OLLAMA_HOST=192.168.88.20:11434 ollama serve
```

On systems where Ollama runs as a service, change its service environment/configuration rather than launching a second competing server. Use an address reachable from the qmanga client. Configure the translation server URL and **model name** in qmanga's translation settings; the base URL will typically resemble:

```text
http://192.168.88.20:11434
```

The integration uses Ollama's `POST /api/generate` endpoint. It requests a non-streaming response and retrieves the generated translation. Model availability, prompt requirements, resource consumption, and translation quality vary; choose a model that supports your source and target languages and verify the results on representative text.

For troubleshooting, you can verify the server from a client machine:

```bash
curl http://192.168.88.20:11434/api/tags
```

If qmanga cannot connect, check the listening address, firewall, selected model, and whether the server is reachable from the client. **Do not expose unauthenticated inference endpoints directly to the public internet.** For networks you do not fully control, use appropriate authentication and encryption (such as an authenticated HTTPS reverse proxy or VPN).

### jpreader on Linux

[jpreader](https://github.com/kernel1024/jpreader) is an optional, separately installed Linux application. When its D-Bus service is available, qmanga can send text for translation and receive the result asynchronously.

jpreader is not required for the Windows build, and its D-Bus-dependent features are unavailable there. Use Ollama for HTTP-based translation instead.

## Typical reading workflow

1. Open a manga or scanned document, navigate to a page, and select **Google Vision** or **PaddleX** as the OCR engine.
2. Trigger **full-page OCR** using the OCR button. Text-region outlines appear when recognition finishes.
3. Hover over a region to see the recognized text and, if configured, its translation below a horizontal separator.
4. Click regions to append their **original OCR text** to the OCR editor, in the order you prefer.
5. Translate the editor's combined text as one passage for additional context, or continue to the next page and repeat.

A translation backend is optional for OCR recognition and visualization. Automatic OCR grouping may not perfectly match individual speech bubbles; the editor allows the reader to correct or assemble text manually.

## Local/LAN deployment example

```text
Thin laptop or Windows desktop
  └── qmanga (Qt 6)
        ├── PaddleX OCR HTTP  ────────┐
        └── Ollama translation HTTP ──┤
                                     ▼
                         GPU-equipped desktop / server
                          ├── PaddleX OCR :8080
                          └── Ollama      :11434
```

Both services can also run on `localhost`. The qmanga client does not need its own discrete GPU for these HTTP backends.

## Troubleshooting

- **OCR button produces no regions:** check that Google Vision credentials are valid or that the PaddleX endpoint includes `/ocr`, is reachable, and has loaded the OCR pipeline.
- **OCR detects multiple lines in one bubble:** this can be normal; the detection result is a set of text regions, not guaranteed speech-bubble segmentation.
- **Text is recognized but not translated:** verify the selected translation backend, endpoint/model settings, and server availability. OCR itself does not depend on translation.
- **jpreader controls are unavailable on Windows:** jpreader's integration uses Linux D-Bus; use Ollama over HTTP instead.
- **Missing readers or Qt SQL drivers:** check the qmake configuration output and ensure the corresponding build dependencies and Qt plugins are installed and deployed.
- **LAN requests fail:** check bind addresses, host firewalls, and whether the endpoint uses HTTP or HTTPS. Do not expose these services publicly without access controls.

## Contributing and issues

Bug reports, feature suggestions, and build feedback are welcome through [GitHub Issues](https://github.com/kernel1024/qmanga/issues). Please include your operating system, Qt/toolchain versions, enabled readers/OCR backends, and relevant error output when reporting problems.

For source code, updates, and project history, see the [qmanga repository](https://github.com/kernel1024/qmanga).

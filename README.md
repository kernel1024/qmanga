# qmanga
Manga and book reader written with Qt.

Multithreaded manga viewer with fast bookshelf support. ZIP, RAR, PDF, DJVU packed files with original images or scanned pages supported. Text book formats also supported: TXT, EPUB, FB2. Supported office documents: RTF, DOC, DOCX, XLS, XLSX, PPT, PPTX, WPS, ODT, ODS, ODP via LibreOffice converter to PDF format.

### Requirements:
* Qt 6.5+ (gui, widgets, dbus (for linux), sql, xml, network) with C++20 support
* libzip
* zlib
* ICU
* MySQL client library and SQLite 3.x+ with Qt drivers for QtSQL
* intel-tbb

### Additional requirements (optional for linux build):
* Poppler 21.3+ - PDF files support
* Tesseract (with recognition data) - offline OCR functionality
* Djvulibre - DJVU files support
* ebook-tools (libepub) - EPUB files support

Google Vision and PaddleX OCR ML engines is supported via TCP connection, https with OAuth for GCP services, and plain http for PaddleX on LAN or localhost.

Running MySQL server is preferred for collection management and search features at top speed. Also SQLite local database is supported.

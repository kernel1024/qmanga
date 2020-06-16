# qmanga
Manga and book reader written with Qt.

Multithreaded manga viewer with fast bookshelf support. ZIP, RAR, PDF, DJVU packed files with original images or scanned pages supported. There is no support for text book formats (such as txt, doc, rtf, fb2 etc).

### Requirements:
* Qt 5.15+ (gui, widgets, dbus (for linux), sql) with C++17 support
* libzip
* zlib
* MySQL client library and SQLite 3.x+ with Qt drivers for QtSQL
* intel-tbb

### Additional requirements (optional for linux build):
* Poppler 0.83+ - PDF files support
* Tesseract (with japanese recognition data) - OCR functionality
* Djvulibre - DJVU files support

Running MySQL server is preferred for collection management and search features at top speed. Also SQLite local database is supported.

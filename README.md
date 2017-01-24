# qmanga
Manga reader written with Qt.

Multithreaded manga viewer with fast library support. ZIP, RAR and PDF packed manga files supported.

### Requirements:
* Qt 5.x (gui, widgets, dbus (for linux), concurrent, sql) with C++11 support
* libmagic (for linux build)
* zziplib
* MySQL client library and SQLite 3.x+ with Qt drivers for QtSQL

### Additional requirements (optional for linux build):
* Poppler - PDF files support
* Tesseract (with japanese recognition data) - OCR functionality

Running MySQL server is preferred for collection management and search features at top speed. Also SQLite local database is supported.

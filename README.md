# qmanga
Manga reader written with Qt.

Multithreaded manga viewer with fast MySQL-based library support. ZIP, RAR and PDF packed manga files supported.

### Requirements:
* Qt 5.x (gui, widgets, dbus (for linux), concurrent) with C++11 support
* libmagic (for linux build)
* zziplib
* MySQL client library

### Additional requirements (optional for linux build):
* Poppler - PDF files support
* Tesseract (with japanese recognition data) - OCR functionality

Running MySQL server needed for collection management and search features.

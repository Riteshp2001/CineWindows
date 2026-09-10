# CineWindows I18n

User-facing QML strings use `qsTr()`. C++ strings use `tr()` or `QCoreApplication::translate()`.

Translations are wired through CMake with `qt_add_translations()`. To refresh the English source file after changing text:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\generate\update_translations.ps1
```

To compile `.ts` files into `.qm` resources:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\generate\update_translations.ps1 -ReleaseOnly
```

# JSON localization

The catalog format follows PixelStudio's `contexts` dictionaries and Rebit's
per-language domain packs. No `.ts` / `.qm` generation is required.

Each `translations/<language>/` directory contains:

- `meta.json`: `language`, `name`, `author`, `contributors`.
- `common.json`: application shell, navigation and shared controls.
- `overview.json`: overview and USB connection UI.
- `device.json`: telemetry, device information and presentation strings.
- `diagnostics.json`: USB log UI.
- `session.json`: device service messages and errors.

A page pack has this structure:

```json
{
  "contexts": {
    "DeviceHero": {
      "CONNECTED VIA USB": "ПОДКЛЮЧЕНО ПО USB"
    }
  }
}
```

Contexts match the QML filename for `qsTr()` and the C++ class for `tr()`.
Keep `%1`, `%2`, etc. intact. English is the base language: QML `qsTr()` and C++ `tr()` use English source
strings, `en` catalogs map those keys to themselves, and `ru` catalogs provide
the Russian translations. Both catalogs cover every key. New page packs are discovered recursively
at configure time and embedded in the executable. Duplicate keys within a
language are disallowed by the catalog tests.

The shell offers runtime language selection and saves it through QSettings.
On first launch the system language is used when available, otherwise English.
QML bindings retranslate without restarting the application. Existing device
log entries retain the language used when they were recorded.

Custom languages and overrides use the same directory layout under
`QStandardPaths::AppDataLocation/translations/<language>/`. Include `meta.json`
for a new language, then any subset of page packs. Restart to discover a newly
installed language. User translations override bundled strings; missing keys
fall back to English and then the source text. Invalid JSON is ignored.

The data directory follows Qt's native platform conventions (Linux XDG data,
macOS Application Support, Windows AppData) rather than a hardcoded home path.
Run `python3 -m unittest discover -s tests -v` to validate catalog coverage,
placeholder consistency and duplicate keys.

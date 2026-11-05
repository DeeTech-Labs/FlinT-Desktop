[Русский оригинал](../ru/ARCHITECTURE.md)

# Flare architecture

The QML shell and reusable visual components remain under `qml/`. Existing icons, fonts, shaders, localization and tray integration are independent of the device protocol.

`src/platform_app/` contains PlatformClient, IgniteHub, Studio, Preview and CompanionService. These services use binary typed method IDs and the shared v1 codecs. They do not expose old textual commands or a compatibility facade to QML.

`src/device/` contains only the reusable Qt Bluetooth/BlueZ hardware lifecycle. `platform/` is the pinned Flint IgniteSDK submodule containing the portable Core, package manager, Runtime, ResourceManager, event queue, renderer, compiler and contracts. The simulator executes that same Core in an isolated temporary directory.

The complete [shared specification](../../platform/README.md) defines installed formats, consent, transactions, trust and explicit unsupported features.

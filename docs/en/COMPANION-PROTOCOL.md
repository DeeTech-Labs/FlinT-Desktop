[Русский оригинал](../ru/COMPANION-PROTOCOL.md)

# Companion protocol v1

The authoritative framing and service contract is documented in [FlinT Platform](../../platform/readme) and `platform/spec/services.json`.

Flare rejects peers with a different development contract revision, missing build identity or insufficient capabilities even when they report protocol 1. There is no textual command fallback.

BLE requires an authenticated encrypted bond. Opening or confirming pairing is restricted to the retained local USB channel. Watch-side grants and Flare's local data policy are independent; both must permit a sensitive data flow.

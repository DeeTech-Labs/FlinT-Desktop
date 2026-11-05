# Flint Flare logo

[Русский оригинал](../ru/BRANDING.md)

## Master

[brand/flare-logo.svg](../../brand/flare-logo.svg) is the canonical vector master with a transparent background. The raster concept has been reconstructed as editable paths and silver facets. No PNG is embedded or used during export.

The master uses `viewBox="0 0 1254 1254"`, plain paths and solid fills. It has no external resources, fonts, scripts, filters or masks. Edit it in Inkscape, Illustrator or another SVG editor. Retain group IDs when they are used by an animation.

![Vector logo on a graphite background](../../brand/icons/flare-app-512.png)

## Layers and facets

The outer group is `flare-mark`. It contains `flare-left`, `flare-core` and `flare-right`. Each includes a base silhouette and individually identified facets: `left-facet-01`…`07`, `core-facet-01`…`05`, `right-facet-01`…`06`.

Inline SVG in a DOM or an animation editor can target groups or individual facets. Set transform origins explicitly. The central lower pivot is approximately `(627, 1018)` in master coordinates.

The [left](../../brand/flare-layer-left.svg), [core](../../brand/flare-layer-core.svg) and [right](../../brand/flare-layer-right.svg) files share dimensions and viewBox, so they align exactly when stacked. They are generated derivatives, not separate editing masters.

## Animation

Animate groups or individual facets in an environment that exposes SVG elements. In Qt Quick, use the separate SVG layers: a regular `Image` renders the SVG as one image and does not expose internal IDs. Stack layers in identical bounds and apply transforms from application code.

The `brand/` directory contains only images and icons. It does not include a Qt implementation or demo scene.

## Platform icons

- **Linux:** the transparent [SVG master](../../brand/flare-logo.svg) or a PNG at the required size. [flare-app-icon.svg](../../brand/flare-app-icon.svg) adds a graphite background.
- **Windows:** [flint-flare.ico](../../brand/icons/flint-flare.ico) contains 16, 24, 32, 48, 64, 128 and 256 pixel representations.
- **macOS:** [flint-flare.icns](../../brand/icons/flint-flare.icns) includes a 1024 pixel representation.
- **Other uses:** transparent `brand/icons/flare-{size}.png` and graphite-backed `brand/icons/flare-app-{size}.png`, at 16, 24, 32, 48, 64, 128, 256, 512 and 1024 pixels.

These are prepared assets. Executable, bundle and package icons have not been switched; connect them in the relevant build/packaging configuration. Platform-specific padding and outer dimensions can be adapted without changing the master.

## Editing and export

The source for the new logo is `brand/flare-logo.svg`. The background variant, separate layers and raster icons are derivatives; update them together after editing the master. Preserve the same viewBox across layers and check small-size readability.

Export SVG to PNG using a vector editor; ICO and ICNS require the appropriate multi-size containers. Ready-to-use platform files live in `brand/icons/`.

[Brand policy](TRADEMARKS.md) · [Interface icons](ICONS.md)

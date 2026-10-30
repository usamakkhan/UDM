# Toolbar image subfolder support

Static inspection of five ZIP packages from the official IDM gallery found a concrete loader gap: Pure Flat and Kavian metadata use relative subfolders for BMP strips, while UDM required every image to sit beside its .tbi file. Small, Windows 10 and MosI3D use adjacent files. The inspected metadata is v3; sampled BMP headers are uncompressed 32-bit images.

The loader now accepts relative BMP subpaths up to eight components and 1,024 bytes. Each component must be a valid filename; absolute/rooted paths, colon/stream syntax, dot/parent components and reparse points are rejected. Missing directories or files produce an error. Existing bitmap size, encoding, frame count and state-image dimension validation remain in place.

The Windows 10 strip is 672 by 42 pixels and visibly contains twelve rectangular frames. No square-frame assumption or frame-count change was introduced. Package inspection is source/format evidence, not a UDM rendering or interaction test. No downloaded skins are bundled into UDM or installed.

Sources: [official gallery](https://www.internetdownloadmanager.com/support/toolbar2.html), [Small](https://www.internetdownloadmanager.com/support/toolbar/small_btns.zip), [Windows 10](https://www.internetdownloadmanager.com/support/toolbar/Windows10.zip), [Pure Flat](https://www.internetdownloadmanager.com/support/toolbar/PureFlat.zip), [MosI3D](https://www.internetdownloadmanager.com/support/toolbar/MosI3D_1.zip), [Kavian](https://www.internetdownloadmanager.com/support/toolbar/Kavian.zip). The local inspection copies are under candidates/toolbar-gallery-reference-20261029; their hashes are recorded with the build evidence.

Extension 0.63.70 is unchanged. All five production targets compiled and installer packaging passed. Installer and payload hashes are recorded in [build evidence](validation/toolbar-subfolder-build-20261029.json) and the production bundle manifest. Runtime, parser and UI tests remain deferred at user request. No installation or publication is performed.

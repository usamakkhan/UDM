# UDM HTTP/2 dependency

Pinned nghttp2 1.70.0 from the [official release](https://github.com/nghttp2/nghttp2/releases/tag/v1.70.0). The release asset's published SHA-256 digest matched the downloaded archive: `e05cb1388eaca3830aded4ccf20044b6e1ac1a61411dcca11b0437c4285c8bc2`. This is HTTPS/download-digest verification, not an independent signature verification.

Only the C library is compiled, statically with the MSVC `/MT` runtime. Applications, examples, documentation, shared libraries and upstream test programs are disabled. No global toolchain or runtime installation is performed. `sources.json` records the input and built-library hashes.

Rebuild through `native/build-curl.ps1` with the pinned curl ZIP, `-Nghttp2Archive` pointing to the pinned tar.xz, a CMake executable, a fresh absolute BuildRoot, and the cached MSVC/SDK ToolchainRoot. The script validates both archive checksums before extraction. It builds nghttp2, then links it into static libcurl. The normal native build links both static libraries.

The MIT license in `LICENSE.txt` must accompany distribution. The native build and installer include it as `nghttp2-LICENSE.txt`.

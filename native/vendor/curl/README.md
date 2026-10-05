# UDM explicit proxy transport dependency

Pinned libcurl 8.22.0, built from the [official source archive](https://curl.se/download/curl-8.22.0.zip) with MSVC 14.44.35207, Windows SDK 10.0.26100.0 and CMake 4.4.3. Source SHA-256: `f9ec970e52124e494606209e6bc0e985c623b428796641742f60c5b56931aaee`.

The detached archive signature was verified against curl's documented release-key fingerprint `27EDEAF22F3ABCEB50DB9A125CC908FDB71E12C2`. The verification library does not check key revocation or self-signatures; the pinned fingerprint and official HTTPS sources provide the independent key binding. CMake's downloaded archive matched its published checksum. See `sources.json` for the downloaded inputs.

This static `/MT` build uses Schannel and the Windows certificate store. It supports HTTP/HTTPS and proxy connections, including HTTP/2 through the pinned nghttp2 1.70.0 library. It does not use a bundled certificate authority file, invoke curl.exe, read .netrc, or enable automatic client certificates. FTP continues to use UDM's existing transport. Compression decoding and automatic redirects are disabled by UDM's adapter; range validation, cookies, cancellation, and redirect policy remain under UDM's control.

`CurlHttp.hpp` selects this transport for explicit HTTP/SOCKS proxy routes to loopback hosts that WinHTTP would bypass, encrypted HTTP proxies, and captured SOCKS routes requiring local DNS. HTTPS requests offer HTTP/2 with HTTP/1.1 fallback; cleartext requests retain HTTP/1.1. It is not a replacement for all WinHTTP connections. No claim about IDM's internal transport follows from this dependency choice.

To rebuild, pass the pinned curl source ZIP, the pinned nghttp2 tar.xz through `-Nghttp2Archive`, a portable CMake executable, a fresh absolute build directory, and the cached MSVC/SDK directory to `native/build-curl.ps1`. Then run the normal `native/build.ps1`. The build script verifies both pinned archive hashes before extraction and does not download or install tools globally.

`LICENSE.txt` must accompany distributed binaries containing this library. The installer includes `curl-LICENSE.txt` alongside UDM.exe.

The HTTP/2 library has its own retained [source provenance and license](../nghttp2/README.md). Negotiation support does not establish multiplexed connection sharing across separate CurlHttp objects or any throughput advantage over another downloader.

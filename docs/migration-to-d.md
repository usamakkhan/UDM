# Development project migration — 19 September 2026

The active UDM project is `D:\UDM`. Native builds, release binaries, browser sources and the extracted compiler cache have been copied there. Its `browser\chromium` path is now a directory junction to `D:\UDM\browser\chromium`, preserving the already-loaded unpacked extension identity/path. The previous directory is retained as `chromium.before-migration`.

Chrome/Chromium/Edge native-host registration points to `D:\UDM\release\Udm.NativeHost.exe`. Version 0.7 binaries are retained in `D:\UDM\backups\0.7.0`. Application history/settings remain in `%LOCALAPPDATA%\UDM`; moving source code does not require moving the user's downloaded files or partial transfers.

`D:\UDM\IDM FIles` is a read-only reference for this work and is excluded from UDM packages. No installed IDM file was patched. UDM does not bundle IDM binaries, extension code, drivers, license data or artwork.

Generated fixtures, local state, media helper binaries and compiler cache are excluded from the portable source ZIP. The optional media setup script obtains verified helpers separately.

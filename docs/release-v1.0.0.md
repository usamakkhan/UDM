# UDM 1.0.0

UDM 1.0.0 is the first major release of the project. It combines the 0.92 content-recognition release with browser-to-desktop capture, authentication, FTP, and Windows automation improvements completed since that release.

## Highlights

- **Safer file publication:** UDM samples download content before publication, recognizes HTML/error pages returned in place of files, pauses safely, and lets the user choose whether to retain received data. It can also add an inferred extension to an extensionless or script-style address without changing a supplied filename.
- **Browser capture rules:** Add site-specific rules under **Options > File Types > Site rules** to ignore, capture, or classify matching browser responses. The browser diagnostics page records the last 50 rule decisions without recording full URLs or request contents.
- **More resilient transfers:** When an HTTP/2 transfer fails during exchange, UDM retries once with HTTP/1.1 and retains that protocol for the remainder of the download.
- **Windows and proxy sign-in:** Saved credentials can answer server challenge schemes; optionally, intranet servers and proxies can use the current Windows login. Explicit proxy credentials cover Basic and Digest authentication.
- **FTP and proxy support:** FTP transfers are qualified for passive/active operation, resume, timestamps, login, and HTTP CONNECT. Gateway proxy modes support common USER, SITE, and OPEN command formats.
- **Windows download automation:** The installer registers `UDM.LinkTransmitter`, allowing Windows callers to send one link or a group of links into UDM. Groups are stored encrypted and shown for review before downloading.
- **Extension setup guidance:** The installer includes a local post-install guide for enabling the Chromium extension without distributing a portable application package.

## Upgrade notes

- Install 1.0.0 over an earlier UDM version; the setup program retains the desktop application, driver, browser integration, and user data according to its normal upgrade path.
- Browser integration remains version 0.63.8. Enable the packaged extension from the setup guide after installation if it is not already enabled.
- The release retains the project’s focus on a free, open-source Windows download manager. It is not a claim of full feature parity with IDM.

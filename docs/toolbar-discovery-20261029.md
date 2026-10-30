# Toolbar discovery workflow

The toolbar menu now has “Look for new toolbars (IDM gallery)” to open the official online gallery in the default browser. “Open toolbar folder” separately opens UDM's own Toolbar directory. Previously the discovery label only opened that local directory. Existing load, reload and built-in style commands are retained.

The new command is appended to the command enumeration, preserving existing command IDs, and routed through both normal command handling and the toolbar popup. Its HTTPS destination is a fixed allowed product-link target. The external gallery is explicitly identified; no third-party skins were downloaded or bundled by this change.

The existing loader supports validated v2/v3 .tbi metadata and adjacent BMP strips. Gallery discovery does not establish that every listed skin is compatible. To use a chosen skin, extract its files into the UDM folder, then use Reload toolbar skins or Load toolbar skin. The user remains responsible for selecting the downloaded package; this change does not automatically install gallery content.

Reference: IDM's [customization instructions](https://www.internetdownloadmanager.com/support/customization.html) describe online toolbar discovery, and its [official gallery](https://www.internetdownloadmanager.com/support/toolbar2.html) supplies toolbar packages. These pages were read during this change; no runtime UDM interaction was performed.

Extension 0.63.70 is unchanged. All five production targets and the unsigned installer built successfully. See the [build record](validation/toolbar-discovery-build-20261029.json) for source and artifact hashes. Runtime/UI/skin-compatibility testing remains deferred at user request. No installation or publication is performed.

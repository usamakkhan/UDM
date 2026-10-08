Follow-up correction: the light tabs/header in this report's image resulted from missing print-client painting, not missing normal dark painting. See [rendering investigation](theme-print-20261008.md).

# Live dialog appearance refresh — 8 October 2026

Open UDM dialogs now refresh button styles and list colors when the main appearance setting changes. The unchanged-code regression reproduced the stale primary button style. The refresh keeps existing windows, text, focus, list selection and action availability, and leaves download state untouched.

Light-mode controls use Windows-owned system brushes and matching button text colors. The effective dark preference yields to Windows high-contrast settings without changing the saved preference. System color and setting notifications refresh existing controls without rebuilding the toolbar or changing Windows settings.

Validation: 61 appearance checks, 97 completion-dialog checks and 188 Properties checks passed (346 total). The appearance fixture exercises four light/dark transitions on an existing Form and actual paused Progress dialog, verifies actions remain callable, and checks current-palette notifications. An intermediate fixture incorrectly assumed its edit acquired focus; diagnostics showed the same non-null focused window before and after refresh. The final assertion verifies preservation of that established focus. Baseline, intermediate and final results are retained.

The application and GUI test entry points were rebuilt against the previously verified recorded-caption backend objects. This is not a full clean backend build. Compiler warnings remain. Raw logs, images and build scripts are under `C:\Users\Abuzar\AppData\Local\Temp\udm-live-appearance-20261008`. The rendered progress image was inspected: tabs and some surfaces remain light in dark mode. This change does not establish complete visual or high-contrast accessibility compliance. No real Windows contrast palette was enabled.

The candidate path and hash are recorded in the [validation receipt](validation/live-appearance-20261008.json). Not packaged, installed or published. The existing installer predates this and the previous DPI-column fix. GitHub HEAD observed during review was 0529e71; these recent changes remain local. Full IDM parity remains unverified.

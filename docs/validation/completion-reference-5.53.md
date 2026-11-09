# Download complete fields and moved-file warning in UDM 5.53

The retained IDM dialog metadata and screenshot show plain-bordered Address and saved-path edits. UDM 5.53 uses that border with read-only, horizontally scrolling Unicode text. The completion fixture still compares control class, text and bounds against the retained dialog at 96, 144 and 192 simulated DPI. [Reference metadata](../reference/completion-dialog.json) · [Reference screenshot](../gui-parity-2026-10-18/idm-download-complete.png)

The fixture moves a completed file after opening its completion window. It clicks Open and Open with and reads the warning on the dialog thread, then dismisses it with its actual OK button. This replaces the optional cross-thread observer that previously timed out. A frozen 5.52 control observed the moved-file warning for Open, a generic warning for Open with, and recessed borders on both edits.

UDM 5.53 uses the existing moved-file explanation for both actions. It retains the completion window after the warning so the user can still read or copy the saved location and close the dialog normally.

All **131 native completion checks passed** on 5.53. They include both moved-file actions, the reference geometry, scanner controls across four simulated DPI values, completion suppression, and a captured response that emitted one completion event. The border assertion measures the painted edges at 96, 144 and 192 simulated DPI. In the fixture, Windows drew the plain outline inside the edit control's client area, so the first style and frame assertions did not describe the rendered result. The receipt records the final qualification fixture separately from the production build inputs. [Build receipt](completion-reference-5.53-build.json)

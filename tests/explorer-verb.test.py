"""Explorer "Download with UDM" verb: both installers register the same Internet Shortcut verb,
pointing at UDM.exe --shortcut "%1", and the setup removes it with the application."""
import pathlib, re, sys

root = pathlib.Path(__file__).resolve().parent.parent
passed = 0


def check(ok, name):
    global passed
    if not ok:
        print("FAIL " + name)
        sys.exit(1)
    passed += 1
    print("PASS " + name)


setup = (root / "installer" / "UDM-Setup.iss").read_text(encoding="utf-8")
registry = setup[setup.index("[Registry]"):setup.index("[Run]")]
verb = r'Software\Classes\InternetShortcut\shell\UDM.Download'
entries = [line for line in registry.splitlines() if line.startswith("Root:")]
check(all(verb in line for line in entries) and len(entries) == 3, "Setup registers exactly the Internet Shortcut verb")
check(any('ValueData: "Download with UDM"' in line and "uninsdeletekey" in line for line in entries), "Setup names the verb Download with UDM and removes it on uninstall")
check(any(verb + r'\command"' in line and 'ValueData: """{app}\\UDM.exe"" --shortcut ""%1"""' in line for line in entries), "Setup's verb command runs UDM.exe --shortcut with the shortcut path")
check(any('ValueName: "Icon"' in line and '{app}\\UDM.exe' in line for line in entries), "Setup gives the verb UDM's icon")
check(all("Root: HKA;" in line for line in entries), "Setup writes the verb in the installer's own scope")

dev = (root / "install.ps1").read_text(encoding="utf-8")
check(r"HKCU:\Software\Classes\InternetShortcut\shell\UDM.Download" in dev and "'Download with UDM'" in dev, "Development installer registers the same verb for the current user")
check(re.search(r"""\\command" -Value \('"' \+ \$exe \+ '" --shortcut "%1"'\)""", dev) is not None, "Development installer's verb command matches the setup")

launch = (root / "native" / "Launch.hpp").read_text(encoding="utf-8")
check('arg==L"--shortcut"' in launch and "readInternetShortcut" in launch, "UDM.exe accepts --shortcut and reads the Internet Shortcut file")
print("ALL %d EXPLORER VERB CHECKS PASSED" % passed)

import os
import re
import sys

def update_version(new_version):
    project_root = os.path.dirname(os.path.abspath(__file__))
    
    # 1. src/utils/constants.py
    constants_path = os.path.join(project_root, "src", "utils", "constants.py")
    if os.path.exists(constants_path):
        with open(constants_path, "r", encoding="utf-8") as f:
            content = f.read()
        content = re.sub(r'APP_VERSION\s*=\s*".*?"', f'APP_VERSION = "{new_version}"', content)
        with open(constants_path, "w", encoding="utf-8") as f:
            f.write(content)
        print(f"Updated {constants_path}")

    # 2. pyproject.toml
    pyproject_path = os.path.join(project_root, "pyproject.toml")
    if os.path.exists(pyproject_path):
        with open(pyproject_path, "r", encoding="utf-8") as f:
            content = f.read()
        content = re.sub(r'version\s*=\s*".*?"', f'version = "{new_version}"', content, count=1)
        with open(pyproject_path, "w", encoding="utf-8") as f:
            f.write(content)
        print(f"Updated {pyproject_path}")

    # 3. scripts/installer.nsi
    installer_path = os.path.join(project_root, "scripts", "installer.nsi")
    if os.path.exists(installer_path):
        with open(installer_path, "r", encoding="utf-8") as f:
            content = f.read()
        content = re.sub(r'!define APP_VERSION\s*".*?"', f'!define APP_VERSION "{new_version}"', content)
        with open(installer_path, "w", encoding="utf-8") as f:
            f.write(content)
        print(f"Updated {installer_path}")

    # 4. src/launcher.rc
    launcher_path = os.path.join(project_root, "src", "launcher.rc")
    if os.path.exists(launcher_path):
        with open(launcher_path, "r", encoding="utf-8") as f:
            content = f.read()
        
        comma_version = new_version.replace(".", ",") + ",0"
        content = re.sub(r'FILEVERSION\s+.*', f'FILEVERSION {comma_version}', content)
        content = re.sub(r'PRODUCTVERSION\s+.*', f'PRODUCTVERSION {comma_version}', content)
        
        content = re.sub(r'VALUE\s+"FileVersion",\s*".*?"', f'VALUE "FileVersion", "{new_version}"', content)
        content = re.sub(r'VALUE\s+"ProductVersion",\s*".*?"', f'VALUE "ProductVersion", "{new_version}"', content)
        
        with open(launcher_path, "w", encoding="utf-8") as f:
            f.write(content)
        print(f"Updated {launcher_path}")

    print(f"\nSuccess! Changed app version to {new_version} across all files.")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        update_version(sys.argv[1])
    else:
        print("Usage: python bump_version.py <version>")
        print("Example: python bump_version.py 1.0.2")

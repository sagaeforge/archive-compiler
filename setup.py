#!/usr/bin/env python3
import os
import platform
import subprocess
import sys
from pathlib import Path

def run_command(cmd, cwd=None):
    try:
        if platform.system().lower() == "windows":
            subprocess.run(cmd, cwd=cwd, check=True, shell=True)
        else:
            subprocess.run(cmd, cwd=cwd, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error executing command: {cmd}")
        print(f"Error details: {e}")
        sys.exit(1)

def read_package_list(project_root):
    package_file = project_root / ".vcpkg-list"
    if not package_file.exists():
        print(f"Warning: {package_file} not found!")
        return []
    
    with open(package_file, 'r') as f:
        # 빈 줄과 주석(#)을 제외한 패키지 목록 읽기
        packages = [line.strip() for line in f 
                   if line.strip() and not line.startswith('#')]
    return packages

def install_packages(vcpkg_root, platform_name, packages):
    if not packages:
        print("No packages to install.")
        return

    print("Installing required packages...")
    
    triplet = {
        "windows": "x64-windows",
        "darwin": "x64-osx",
        "linux": "x64-linux"
    }[platform_name]
    
    vcpkg_exe = "vcpkg.exe" if platform_name == "windows" else "./vcpkg"
    
    for package in packages:
        print(f"Installing {package}...")
        cmd = f"{vcpkg_exe} install {package}:{triplet}"
        run_command(cmd, cwd=str(vcpkg_root))

def setup_vcpkg():
    project_root = Path(__file__).parent
    vcpkg_root = project_root / "external" / "vcpkg"
    platform_name = platform.system().lower()
    
    print(f"Bootstrapping VCPKG for {platform_name}...")
    print(f"VCPKG path: {vcpkg_root}")
    
    # Bootstrap
    if platform_name == "windows":
        bootstrap_script = str(vcpkg_root / "bootstrap-vcpkg.bat")
    else:
        bootstrap_script = "./bootstrap-vcpkg.sh"
        bootstrap_path = vcpkg_root / "bootstrap-vcpkg.sh"
        bootstrap_path.chmod(bootstrap_path.stat().st_mode | 0o755)

    run_command(bootstrap_script, cwd=str(vcpkg_root))
    
    # Read and install packages
    packages = read_package_list(project_root)
    install_packages(vcpkg_root, platform_name, packages)

if __name__ == "__main__":
    try:
        setup_vcpkg()
        print("VCPKG setup completed successfully!")
    except Exception as e:
        print(f"Error during VCPKG setup: {e}")
        sys.exit(1)
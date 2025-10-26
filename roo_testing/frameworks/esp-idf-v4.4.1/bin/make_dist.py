#!/usr/bin/env python3
"""
Script to build all Bazel targets and combine ESP-IDF archive files into a single distribution archive.

This script:
1. Runs 'bazel build ...' to build all targets (unless --skip-build is specified)
2. Finds all archive files (.a) under bazel-bin/roo_testing/frameworks/esp-idf-v4.4.1
3. Extracts all object files from these archives
4. Creates a new combined archive libesp-idf-v4.4.1.a in the dist/ directory

Usage:
    python3 make_dist.py [--skip-build] [--target TARGET] [--help]
    
Options:
    --skip-build    Skip the 'bazel build' step and use existing archives
    --target TARGET Specify a specific bazel target instead of '...'
    --help          Show this help message
    
Examples:
    # Build all targets and create distribution archive
    python3 make_dist.py
    
    # Use existing archives (skip build step)
    python3 make_dist.py --skip-build
    
    # Build specific target and create distribution
    python3 make_dist.py --target //roo_testing/frameworks/esp-idf-v4.4.1/...
"""

import os
import sys
import subprocess
import tempfile
import shutil
import glob
import argparse
from pathlib import Path


def run_command(cmd, cwd=None, check=True):
    """Run a shell command and return the result."""
    print(f"Running: {' '.join(cmd) if isinstance(cmd, list) else cmd}")
    result = subprocess.run(
        cmd, 
        shell=isinstance(cmd, str), 
        cwd=cwd, 
        capture_output=True, 
        text=True, 
        check=check
    )
    if result.stdout:
        print(result.stdout)
    if result.stderr:
        print(result.stderr, file=sys.stderr)
    return result


def find_workspace_root():
    """Find the Bazel workspace root by looking for WORKSPACE or MODULE.bazel file."""
    current = Path.cwd()
    while current != current.parent:
        if (current / "WORKSPACE").exists() or (current / "MODULE.bazel").exists():
            return current
        current = current.parent
    raise RuntimeError("Could not find Bazel workspace root (no WORKSPACE or MODULE.bazel file found)")


def main():
    # Parse command line arguments
    parser = argparse.ArgumentParser(description="Build and combine ESP-IDF archives")
    parser.add_argument("--skip-build", action="store_true", 
                        help="Skip the 'bazel build' step and use existing archives")
    parser.add_argument("--target", default="roo_testing/frameworks/esp-idf-v4.4.1/...", 
                        help="Specify bazel target to build (default: 'roo_testing/frameworks/esp-idf-v4.4.1/...')")
    args = parser.parse_args()
    
    # Get the directory where this script is located
    script_dir = Path(__file__).parent.absolute()
    esp_idf_dir = script_dir.parent
    
    # Find the Bazel workspace root
    try:
        workspace_root = find_workspace_root()
        print(f"Found Bazel workspace at: {workspace_root}")
    except RuntimeError as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)
    
    # Change to workspace root for bazel command
    os.chdir(workspace_root)
    
    # Step 1: Build targets (unless skipped)
    if not args.skip_build:
        print(f"Step 1: Building targets '{args.target}' with Bazel...")
        try:
            run_command(["bazel", "build", args.target])
        except subprocess.CalledProcessError as e:
            print(f"Error: Bazel build failed with exit code {e.returncode}", file=sys.stderr)
            print("You can retry with --skip-build to use existing archives", file=sys.stderr)
            sys.exit(1)
    else:
        print("Step 1: Skipping build (using existing archives)")
    
    print("\nStep 2: Finding archive files...")
    
    # Path to the bazel-bin directory containing ESP-IDF archives
    bazel_bin_esp_idf = workspace_root / "bazel-bin" / "roo_testing" / "frameworks" / "esp-idf-v4.4.1"
    
    if not bazel_bin_esp_idf.exists():
        print(f"Error: Expected directory not found: {bazel_bin_esp_idf}", file=sys.stderr)
        sys.exit(1)
    
    # Find all .a files
    archive_files = []
    for root, dirs, files in os.walk(bazel_bin_esp_idf):
        for file in files:
            if file.endswith('.a'):
                archive_files.append(Path(root) / file)
    
    if not archive_files:
        print("Error: No archive files (.a) found in bazel-bin output", file=sys.stderr)
        sys.exit(1)
    
    print(f"Found {len(archive_files)} archive files:")
    for archive in sorted(archive_files):
        print(f"  {archive.relative_to(bazel_bin_esp_idf)}")
    
    print("\nStep 3: Creating combined archive...")
    
    # Ensure dist directory exists
    dist_dir = esp_idf_dir / "dist"
    dist_dir.mkdir(exist_ok=True)
    
    # Output archive path
    output_archive = dist_dir / "libesp-idf-v4.4.1.a"
    
    # Remove existing output archive if it exists
    if output_archive.exists():
        output_archive.unlink()
        print(f"Removed existing archive: {output_archive}")
    
    # Create temporary directory for extracting objects
    with tempfile.TemporaryDirectory() as temp_dir:
        temp_path = Path(temp_dir)
        print(f"Using temporary directory: {temp_path}")
        
        # Extract all object files from all archives
        object_files = []
        for i, archive in enumerate(archive_files):
            # Create a subdirectory for this archive to avoid filename conflicts
            archive_temp_dir = temp_path / f"archive_{i:03d}_{archive.stem}"
            archive_temp_dir.mkdir()
            
            print(f"Extracting {archive.name}...")
            try:
                # Extract objects from this archive
                run_command(["ar", "x", str(archive)], cwd=archive_temp_dir)
                
                # Find all extracted object files
                extracted_objects = list(archive_temp_dir.glob("*.o"))
                if extracted_objects:
                    object_files.extend(extracted_objects)
                    print(f"  Extracted {len(extracted_objects)} object files")
                else:
                    print(f"  Warning: No object files found in {archive.name}")
                    
            except subprocess.CalledProcessError as e:
                print(f"Warning: Failed to extract from {archive.name}: {e}", file=sys.stderr)
                continue
        
        if not object_files:
            print("Error: No object files were extracted from any archive", file=sys.stderr)
            sys.exit(1)
        
        print(f"\nTotal object files to combine: {len(object_files)}")
        
        # Create the combined archive
        print(f"Creating combined archive: {output_archive}")
        
        # Use ar to create the archive with all object files
        # We'll do this in batches to avoid command line length limits
        batch_size = 100
        
        # Create initial archive with first batch
        first_batch = object_files[:batch_size]
        remaining_objects = object_files[batch_size:]
        
        run_command(["ar", "cr", str(output_archive)] + [str(obj) for obj in first_batch])
        
        # Add remaining objects in batches
        while remaining_objects:
            batch = remaining_objects[:batch_size]
            remaining_objects = remaining_objects[batch_size:]
            run_command(["ar", "r", str(output_archive)] + [str(obj) for obj in batch])
        
        # Create an index for the archive
        run_command(["ranlib", str(output_archive)])
    
    # Verify the created archive
    print(f"\nStep 4: Verifying created archive...")
    result = run_command(["ar", "t", str(output_archive)], check=False)
    if result.returncode == 0:
        object_count = len(result.stdout.strip().split('\n')) if result.stdout.strip() else 0
        print(f"Successfully created {output_archive}")
        print(f"Archive contains {object_count} object files")
        
        # Show archive size
        size_mb = output_archive.stat().st_size / (1024 * 1024)
        print(f"Archive size: {size_mb:.1f} MB")
    else:
        print("Error: Failed to verify the created archive", file=sys.stderr)
        sys.exit(1)
    
    print(f"\nDistribution archive created successfully: {output_archive}")


if __name__ == "__main__":
    main()
#!/bin/bash
set -e  # Exit immediately if a command exits with a non-zero status

# Configuration
PROTOC_PATH="protoc"       # Path to the protoc executable
PROTO_SRC="./proto_file"    # Directory containing proto files
OUT_DIR="./generated_src"             # Output directory for compiled files
INCLUDE_DIR="./generated_include"     # Target directory for header files

# Clean output directories
echo "Cleaning output directories..."
rm -rf "$OUT_DIR" "$INCLUDE_DIR"
mkdir -p "$OUT_DIR"
mkdir -p "$INCLUDE_DIR"

# Compile proto files
find "$PROTO_SRC" -name "*.proto" | while read -r proto_file; do
    echo "Compiling $proto_file ..."
    "$PROTOC_PATH" \
        --proto_path="$PROTO_SRC" \
        --cpp_out="$OUT_DIR" \
        "$proto_file"
done

echo "Compilation finished."

# Move all header files to include directory
find "$OUT_DIR" -name "*.h" | while read -r header_file; do
    # Get path relative to OUT_DIR
    relative_path="${header_file#$OUT_DIR/}"
    target_dir="$INCLUDE_DIR/$(dirname "$relative_path")"
    mkdir -p "$target_dir"
    mv "$header_file" "$target_dir/"
done

echo "All header files moved to $INCLUDE_DIR."

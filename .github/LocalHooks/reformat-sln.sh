#!/bin/sh

VS_PATH=`"C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vswhere.exe" -latest -property installationPath`
CLANG_FORMAT="${VS_PATH}\\VC\\Tools\\Llvm\\bin\\clang-format.exe"

STYLE=$(git config --get hooks.clangformat.style)

if [ -n "${STYLE}" ] ; then
  STYLEARG="-style=${STYLE}"
else
  STYLEARG=""
fi

format_file() {
  file="${1}"
  if [ -f $file ]; then
    echo "Formatting: " $file
    "${CLANG_FORMAT}" -i ${STYLEARG} $file
  
    git add $file
  else
    echo "Failed to find file: " $file
  fi
}

echo "Runs clang-format on source files...."

cd ../../

find . -type f \( -name "*.cpp" -o -name "*.cc" -o -name "*.h" -o -name "*.hpp" \) | while read FILE
do
     # Look for .clang-format file in the current directory
  dir=$(dirname "$FILE")
  clang_format_file="$dir/.clang-format"
  
  if [ -f "$clang_format_file" ]; then
    STYLEARG="-style=file"
  else
    # If .clang-format not found in current directory, use the one in the root
    STYLEARG=""
  fi
  
  format_file "$FILE"
done
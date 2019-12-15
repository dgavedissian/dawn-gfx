#!/usr/bin/env bash
if hash clang-format-9 2>/dev/null; then
    CLANG_FORMAT=clang-format-9
else
    CLANG_FORMAT=clang-format
fi

# Check the version is correct.
CLANG_FORMAT_VERSION=$("${CLANG_FORMAT}" --version | cut -d " " -f 3)
if [[ ! "${CLANG_FORMAT_VERSION}" =~ ^9.0 ]]; then
    echo "Unsupported clang-format version ${CLANG_FORMAT_VERSION}, expected 9.0"
    exit
fi

# Run clang-format against a set of paths.
PATHS=(
    include
    src
)
find "${PATHS[@]}" -name "*.h" -o -name "*.cpp" -not -path "./src/dawn-gfx/gl/glad/*" -exec $CLANG_FORMAT -i {} \;

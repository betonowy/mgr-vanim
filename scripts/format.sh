#!/usr/bin/bash

FILES=$(find ./res/ -iregex '.+\.glsl')
FILES="$FILES $(find ./res/ -iregex '.+\.vert')"
FILES="$FILES $(find ./res/ -iregex '.+\.frag')"
FILES="$FILES $(find ./app/ -iregex '.+\.c')"
FILES="$FILES $(find ./app/ -iregex '.+\.cpp')"
FILES="$FILES $(find ./app/ -iregex '.+\.h')"
FILES="$FILES $(find ./app/ -iregex '.+\.hpp')"
FILES="$FILES $(find ./lib/ -iregex '.+\.c')"
FILES="$FILES $(find ./lib/ -iregex '.+\.cpp')"
FILES="$FILES $(find ./lib/ -iregex '.+\.h')"
FILES="$FILES $(find ./lib/ -iregex '.+\.hpp')"

clang-format $FILES -i --verbose

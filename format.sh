#!/usr/bin/env bash
# Format with clang-format
find main -iname *.h -o -iname *.c | xargs clang-format -i


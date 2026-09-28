#!/usr/bin/env bash

set -xe
split_filename="inc/badrtos_split.h"

for file in inc/badrtos_platform_*.h; do
    [ -e "$file" ] || continue

    platform="${file#*platform_}"
    full_header_filename="for_users/badrtos_$platform"

    awk '
      # File 1: Read extracted sections into block strings
      FNR == NR {
        # Code block extraction
        if (/\/\/\ \/\/\/CODE_COPY_START/) { copy_code=1; next }
        if (/\/\/\ \/\/\/CODE_COPY_END/)   { copy_code=0; next }
        if (copy_code) code_block = (code_block ? code_block "\n" : "") $0

        # Doc block extraction
        if (/\/\/\ \/\/\/DOC_COPY_START/) { copy_doc=1; next }
        if (/\/\/\ \/\/\/DOC_COPY_END/)   { copy_doc=0; next }
        if (copy_doc) doc_block = (doc_block ? doc_block "\n" : "") $0
        next
      }

      # File 2: Replace target blocks in split_filename
      /\/\/\ \/\/\/CODE_REPLACE_START/ {
        print $0
        print code_block
        skip=1
        next
      }
      /\/\/\ \/\/\/CODE_REPLACE_END/ {
        skip=0
      }

      /\/\/\ \/\/\/DOC_REPLACE_START/ {
        print $0
        print doc_block
        skip=1
        next
      }
      /\/\/\ \/\/\/DOC_REPLACE_END/ {
        skip=0
      }

      !skip
    ' "$file" "$split_filename" > "$full_header_filename"
done
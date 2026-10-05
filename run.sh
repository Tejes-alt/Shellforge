#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"
if [[ ! -x ./shellforge ]]; then
  make
fi
exec ./shellforge

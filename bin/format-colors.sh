#!/bin/sh

# Prepares stdin input YAML for gperf consumption

echo "struct rgb { const char *name; uint8_t red; uint8_t green; uint8_t blue; };"
echo "%%"
tr -d '[]"' | tr ":" ","

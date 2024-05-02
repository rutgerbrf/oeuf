#!/usr/bin/env bash

nix flake show --json | jq -r '.packages.[].[].name | values | ".#\(.)"' | xargs nix build --rebuild

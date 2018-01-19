#!/usr/bin/env bash
# -*- mode: sh; sh-shell: bash; coding: utf-8; -*-

usage () {
    echo "Usage: doxygen.bash EXECUTABLE ARGS..."
    echo ""
    echo "FIXME: doc"
    exit 1
}

{ (( $# >= 1 )) && [[ -x "${1}" ]]; } || usage

DOXYGEN_EXECUTABLE="${1}"
shift 1
DOXYGEN_ARGS="${*}"

{ eval "${DOXYGEN_EXECUTABLE} ${DOXYGEN_ARGS}" |& cat; } \
    | sed 's/\\n/\n/g' \
    | sed "s/\\\\e/$(echo -e '\e')/g"

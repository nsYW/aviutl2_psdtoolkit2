#!/bin/bash
SCRIPT_DIR=$(cd $(dirname ${0}); pwd)
cd "${SCRIPT_DIR}"

# find application name from csv
APP_NAME="Noname"
while IFS=, read -a cols; do
  if [[ "${cols[0]}" == "#"* ]]; then
    continue
  fi
  if [[ "${cols[0]}" != "" ]] && [[ "${cols[1]}" == "" ]]; then
    APP_NAME="${cols[0]}"
    break
  fi
done < "${SCRIPT_DIR}/langs.csv"

# generate *.pot from C sources
POTFILE="${SCRIPT_DIR}/current.pot"
C_POT="${SCRIPT_DIR}/c_temp.pot"
GO_POT="${SCRIPT_DIR}/go_temp.pot"

xgettext \
  --add-comments="trans:" \
  --from-code="UTF-8" \
  --package-name="${APP_NAME}" \
  --package-version="$(git tag --points-at HEAD | grep . || echo "vX.X.X") ( $(git rev-parse --short HEAD | grep . || echo "unknown") ) " \
  --copyright-holder="${APP_NAME} Developers" \
  --no-wrap --sort-by-file --output "${C_POT}" ../c/*.c
if [ $? -ne 0 ]; then
  echo "failed to generate pot from C sources."
  exit 1
fi

# generate *.pot from Go sources
# xgettext doesn't natively support Go, but we can use Python mode with custom keywords
# Gettext: simple translation (like gettext)
# Pgettext:1c,2: context translation (like pgettext) - 1st arg is context, 2nd is msgid
find ../go -name "*.go" -print0 | xargs -0 xgettext \
  --add-comments="trans:" \
  --from-code="UTF-8" \
  --language=Python \
  --keyword=Gettext \
  --keyword=Pgettext:1c,2 \
  --package-name="${APP_NAME}" \
  --package-version="$(git tag --points-at HEAD | grep . || echo "vX.X.X") ( $(git rev-parse --short HEAD | grep . || echo "unknown") ) " \
  --copyright-holder="${APP_NAME} Developers" \
  --no-wrap --sort-by-file --output "${GO_POT}" 2>/dev/null

# merge C and Go pot files
if [ -f "${GO_POT}" ] && [ -s "${GO_POT}" ]; then
  msgcat --no-wrap --sort-by-file --use-first "${C_POT}" "${GO_POT}" > "${POTFILE}"
else
  cp "${C_POT}" "${POTFILE}"
fi
rm -f "${C_POT}" "${GO_POT}"

# generate *.po
while IFS=, read -a cols; do
  if [[ "${cols[0]}" == "#"* ]] || [[ "${cols[1]}" == "" ]]; then
    continue
  fi
  poname="${cols[0]}.po"
  po="${SCRIPT_DIR}/${poname}"
  if [ -e "${po}" ]; then
    echo "already exists: ${poname}"
    continue
  fi
  porepo="${po}.DO_NOT_EDIT"
  if [ -e "${porepo}" ]; then
    msgmerge --no-wrap --output - "${porepo}" "${POTFILE}" > ${po}
  else
    msginit --no-translator --input="${POTFILE}" --no-wrap --locale=${cols[0]}.UTF-8 --output-file=- > ${po} 2> /dev/null
  fi
  if [ $? -ne 0 ]; then
    echo "failed to generate: ${poname}"
    continue
  fi
  echo "generated: ${poname}"
done < "${SCRIPT_DIR}/langs.csv"

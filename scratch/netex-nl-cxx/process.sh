#!/usr/bin/env bash

set -eu
set -o pipefail

XSD_TAG="v9.3.0-draft1"

rm -rf ./gen ./vendor/src
mkdir -p gen vendor/src

curl -L "https://github.com/BISONNL/NeTEx-NL/archive/refs/tags/$XSD_TAG.tar.gz" -o ./vendor/netex-nl.tar.gz
tar -xzf ./vendor/netex-nl.tar.gz --strip-components 1 -C ./vendor/src/

xsltproc -o ./vendor/src/xsd/netex-nl-basic-new.xsd ./merge-enums-into-basic.xslt ./vendor/src/xsd/netex-nl-basic.xsd
xsltproc -o ./vendor/src/xsd/netex-nl-data-new.xsd ./remove-enums-include.xslt ./vendor/src/xsd/netex-nl-data.xsd
xsltproc -o ./vendor/src/xsd/netex-nl-new.xsd ./remove-enums-include.xslt ./vendor/src/xsd/netex-nl.xsd
rm ./vendor/src/xsd/netex-nl-enums.xsd
mv ./vendor/src/xsd/netex-nl-basic-new.xsd ./vendor/src/xsd/netex-nl-basic.xsd
mv ./vendor/src/xsd/netex-nl-data-new.xsd ./vendor/src/xsd/netex-nl-data.xsd
mv ./vendor/src/xsd/netex-nl-new.xsd ./vendor/src/xsd/netex-nl.xsd

process () {
	xsdcxx cxx-tree --output-dir ./gen --namespace-map http://www.opengis.net/gml/3.2=gml "vendor/src/xsd/$1.xsd"
}

process gml-bison
process netex-nl-basic
process netex-nl-data
process netex-nl

#!/usr/bin/env bash

set -eu
set -o pipefail

export AWS_ACCESS_KEY_ID="$S3_ACCESS_KEY_ID"
export AWS_SECRET_ACCESS_KEY="$S3_SECRET_ACCESS_KEY"

set +x
all_files=()
declare -A metafiles

while IFS=' ' read -r size filename; do
	if [[ "$filename" == *.parquet.meta.json ]]; then
		metafiles["$filename"]=1
	else
		all_files+=($filename)
	fi
done < <(rclone ls \
	--s3-provider "$S3_PROVIDER" \
	--s3-region "$S3_REGION" \
	--s3-endpoint "$S3_ENDPOINT" \
	--s3-env-auth \
	:s3:$S3_BUCKET)

files=()
for filename in "${all_files[@]}"; do
	if [[ -v metafiles["$filename.meta.json"] ]]; then
		files+=($filename)
	fi
done

echo "Found ${#files[@]} relevant KV6 Parquet files"
echo "Synching this directory with these files"

printf "%s\n" "${files[@]}" | rclone copy \
 	--s3-provider "$S3_PROVIDER" \
 	--s3-region "$S3_REGION" \
 	--s3-endpoint "$S3_ENDPOINT" \
 	--s3-env-auth \
	--progress \
 	--files-from - \
 	:s3:$S3_BUCKET ./

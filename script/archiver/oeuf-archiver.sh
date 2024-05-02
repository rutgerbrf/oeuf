#!/usr/bin/env bash

set -eux
set -o pipefail

# This option prevents the loop from running
# if it does not match any files
shopt -s nullglob

oeuf-bundleparquet

export AWS_ACCESS_KEY_ID="$S3_ACCESS_KEY_ID"
set +x  # Don't print the secret access key to the log
export AWS_SECRET_ACCESS_KEY="$S3_SECRET_ACCESS_KEY"
set -x

for file in ./merged/oeuf-*.parquet; do
	rclone move \
		--s3-provider "$S3_PROVIDER" \
		--s3-region "$S3_REGION" \
		--s3-endpoint "$S3_ENDPOINT" \
		--s3-env-auth \
		$file.meta.json :s3:$S3_BUCKET \
	&& \
	rclone move \
		--s3-provider "$S3_PROVIDER" \
		--s3-region "$S3_REGION" \
		--s3-endpoint "$S3_ENDPOINT" \
		--s3-env-auth \
		$file :s3:$S3_BUCKET
done

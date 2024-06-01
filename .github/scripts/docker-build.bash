#!/usr/bin/env bash
set -ex

DOCKERHUB_USERNAME="${DOCKERHUB_USERNAME:-conky}"
DOCKERHUB_IMAGE_ID=$DOCKERHUB_USERNAME/$IMAGE_NAME

# Change all uppercase to lowercase
DOCKERHUB_IMAGE_ID=$(echo $DOCKERHUB_IMAGE_ID | tr '[A-Z]' '[a-z]')

image_tags=()

# Strip git ref prefix from version
VERSION_TAG=$(echo $GITHUB_REF | sed -e 's,.*/\(.*\),\1,')

# Strip "v" prefix from tag name
if [[ "$GITHUB_REF" == refs/tags/* ]]; then
    VERSION_TAG=$(echo $VERSION_TAG | sed -e 's/^v//')
fi

image_tags+=("--tag" "$DOCKERHUB_IMAGE_ID:$VERSION_TAG")

# tag as latest on releases
if [[ "$RELEASE" == ON ]]; then
    image_tags+=("--tag" "$DOCKERHUB_IMAGE_ID:latest")
fi

# Only build amd64 on PRs, build all platforms on main. The arm builds
# take far too long.
image_platforms="--platform linux/amd64"
push_image=""
cache_tag="pr-cache"

# Only push on main
if [[ "$GITHUB_REF" == refs/heads/main ]]; then
    push_image="--push"
    image_platforms="--platform linux/arm/v7,linux/arm64/v8,linux/amd64"
    cache_tag="main-cache"
fi

docker buildx build \
    ${push_image} \
    ${image_platforms} \
    --cache-from=type=registry,ref=$DOCKERHUB_USERNAME/$IMAGE_NAME:$cache_tag \
    --cache-to=type=registry,ref=$DOCKERHUB_USERNAME/$IMAGE_NAME:$cache_tag,mode=max \
    "${image_tags[@]}" \
    .

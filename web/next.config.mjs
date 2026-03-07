// @ts-check
import { execFileSync } from 'node:child_process'

function getGitHash() {
  try {
    return execFileSync('git', ['rev-parse', '--short', 'HEAD'], {
      encoding: 'utf8',
    }).trim()
  } catch {
    return ''
  }
}

const modifiedDate = new Date().toISOString()
const gitHash = getGitHash()

/**
 * @type {import('next').NextConfig}
 */
const nextConfig = {
  output: 'export',
  env: {
    NEXT_PUBLIC_BUILD_DATE: modifiedDate,
    NEXT_PUBLIC_BUILD_YEAR: String(new Date(modifiedDate).getUTCFullYear()),
    NEXT_PUBLIC_GIT_HASH: gitHash,
  },
}

export default nextConfig

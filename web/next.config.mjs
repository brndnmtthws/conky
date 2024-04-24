// @ts-check
import { spawn } from 'child_process'

const config = async (_phase, { _defaultConfig }) => {
  const gitHash = await new Promise((resolve) => {
    const git = spawn('git', ['rev-parse', '--short', 'HEAD'])
    git.stdout.on('data', (data) => {
      resolve(data.toString().trim())
    })
  })

  /**
   * @type {import('next').NextConfig}
   */
  const nextConfig = {
    publicRuntimeConfig: {
      modifiedDate: new Date().toISOString(),
      modifiedYear: new Date().getFullYear(),
      gitHash,
    },
  }
  return nextConfig
}

export default config

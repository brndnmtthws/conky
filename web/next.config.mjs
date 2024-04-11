// @ts-check

/**
 * @type {import('next').NextConfig}
 */
const nextConfig = {
  publicRuntimeConfig: {
    modifiedDate: new Date().toISOString(),
    modifiedYear: new Date().getFullYear(),
  },
}

export default nextConfig

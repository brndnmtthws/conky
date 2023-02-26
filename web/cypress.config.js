/* eslint-disable @typescript-eslint/no-var-requires */
const { defineConfig } = require('cypress')

module.exports = defineConfig({
  scrollBehavior: 'center',
  e2e: {
    baseUrl: 'http://localhost:3000',
    supportFile: false,
  },
})

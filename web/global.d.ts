declare module 'next/config' {
  type ConfigTypes = () => {
    publicRuntimeConfig: {
      modifiedDate: string
      modifiedYear: string
      gitHash: string?
    }
  }

  declare const getConfig: ConfigTypes

  export default getConfig
}

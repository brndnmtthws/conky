function getEnv(name: string, fallback = '') {
  return process.env[name] ?? fallback
}

export const buildMeta = {
  date: getEnv('NEXT_PUBLIC_BUILD_DATE'),
  year: getEnv(
    'NEXT_PUBLIC_BUILD_YEAR',
    String(new Date().getUTCFullYear())
  ),
  gitHash: getEnv('NEXT_PUBLIC_GIT_HASH'),
}

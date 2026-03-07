import js from '@eslint/js'
import nextPlugin from '@next/eslint-plugin-next'
import tsPlugin from '@typescript-eslint/eslint-plugin'
import tsParser from '@typescript-eslint/parser'
import { defineConfig, globalIgnores } from 'eslint/config'
import prettier from 'eslint-config-prettier'
import jsxA11y from 'eslint-plugin-jsx-a11y'
import reactHooks from 'eslint-plugin-react-hooks'
import globals from 'globals'

const tsFiles = ['**/*.{ts,tsx,mts,cts}']
const tsRecommendedTypeChecked = tsPlugin.configs[
  'flat/recommended-type-checked'
].map((config) => (config.files ? config : { ...config, files: tsFiles }))

export default defineConfig([
  {
    files: ['**/*.{js,jsx,mjs,cjs,ts,tsx,mts,cts}'],
    languageOptions: {
      ecmaVersion: 'latest',
      sourceType: 'module',
      parserOptions: {
        ecmaFeatures: {
          jsx: true,
        },
      },
      globals: {
        ...globals.browser,
        ...globals.node,
      },
    },
  },
  js.configs.recommended,
  ...tsRecommendedTypeChecked,
  {
    files: tsFiles,
    languageOptions: {
      parser: tsParser,
      parserOptions: {
        projectService: true,
        tsconfigRootDir: import.meta.dirname,
      },
    },
    plugins: {
      '@next/next': nextPlugin,
      'jsx-a11y': jsxA11y,
      'react-hooks': reactHooks,
    },
    rules: {
      ...reactHooks.configs.flat['recommended-latest'].rules,
      ...jsxA11y.configs.strict.rules,
      ...nextPlugin.configs['core-web-vitals'].rules,
      '@typescript-eslint/consistent-type-imports': 'error',
      '@typescript-eslint/no-empty-object-type': 'off',
      '@typescript-eslint/no-unsafe-assignment': 'off',
      '@typescript-eslint/no-unnecessary-template-expression': 'off',
      '@typescript-eslint/require-await': 'off',
      '@typescript-eslint/no-unused-vars': [
        'error',
        {
          argsIgnorePattern: '^_',
          caughtErrorsIgnorePattern: '^_',
          destructuredArrayIgnorePattern: '^_',
          varsIgnorePattern: '^_',
        },
      ],
      curly: ['error', 'all'],
      eqeqeq: ['error', 'always'],
      'no-var': 'error',
      'object-shorthand': ['error', 'always'],
      'prefer-const': [
        'error',
        {
          destructuring: 'all',
        },
      ],
    },
  },
  {
    files: ['**/*.{js,jsx,mjs,cjs}'],
    ...tsPlugin.configs['flat/disable-type-checked'],
    rules: {
      '@typescript-eslint/no-require-imports': 'off',
    },
  },
  prettier,
  globalIgnores([
    '.next/**',
    '.netlify/**',
    'cypress/**',
    'cypress.config.js',
    'node_modules/**',
    'out/**',
    'public/static/**',
  ]),
])

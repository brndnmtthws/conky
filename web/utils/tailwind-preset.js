import plugin = require('tailwindcss/plugin')
import pluginTypography = require('@tailwindcss/typography')

const hoveredSiblingPlugin = plugin(function ({ addVariant, e }) {
  addVariant('hovered-sibling', ({ container }) => {
    container.walkRules((rule) => {
      rule.selector = `:hover + .hovered-sibling\\:${rule.selector.slice(1)}`
    })
  })
})

module.exports = {
  theme: {
    fontFamily: {
      sans: [
        'Inter Variable',
        'ui-sans-serif',
        'system-ui',
        '-apple-system',
        'BlinkMacSystemFont',
        'Segoe UI',
        'Roboto',
        'Helvetica Neue',
        'Arial',
        'Noto Sans',
        'sans-serif',
        'Apple Color Emoji',
        'Segoe UI Emoji',
        'Segoe UI Symbol',
        'Noto Color Emoji',
      ],
      serif: [
        'Newsreader Variable',
        'ui-serif',
        'Georgia',
        'Cambria',
        'Times New Roman',
        'Times',
        'serif',
      ],
      mono: [
        'Fira Code Variable',
        'ui-monospace',
        'SFMono-Regular',
        'Menlo',
        'Monaco',
        'Consolas',
        'Liberation Mono',
        'Courier New',
        'monospace',
      ],
    },
    extend: {
      typography: (theme) => ({
        DEFAULT: {
          css: {
            code: {
              '&::before': {
                content: '"" !important',
              },
              '&::after': {
                content: '"" !important',
              },
            },
            fontFamily: theme('fontFamily.sans').join(', '),
          },
        },
      }),
    },
  },
  variants: {
    extend: {
      borderRadius: ['first', 'last'],
      borderWidth: ['last', 'hovered-sibling'],
      typography: ['dark'],
    },
  },
  plugins: [hoveredSiblingPlugin, pluginTypography],
}

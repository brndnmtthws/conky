module.exports = {
  content: [
    './pages/**/*.{js,ts,jsx,tsx,mdx}',
    './components/**/*.{js,ts,jsx,tsx,mdx}',
    './utils/**/*.{js,ts,jsx,tsx,mdx}',
    './styles/**/*.{css}',
  ],
  darkMode: 'class',
  presets: [require('./utils/tailwind-preset')],
}

describe('check index renders', () => {
  beforeEach(() => {
    cy.visit('/')
  })

  it('displays the title text', () => {
    cy.get('h1').contains('Conky')
  })
})
describe('check config settings', () => {
  beforeEach(() => {
    cy.visit('/config_settings')
  })

  it('displays the title text', () => {
    cy.get('h1').contains('Configuration settings')
  })
})
describe('check variables', () => {
  beforeEach(() => {
    cy.visit('/variables')
  })

  it('displays the title text', () => {
    cy.get('h1').contains('Variables')
  })
})
describe('check lua', () => {
  beforeEach(() => {
    cy.visit('/lua')
  })

  it('displays the title text', () => {
    cy.get('h1').contains('Lua API')
  })
})

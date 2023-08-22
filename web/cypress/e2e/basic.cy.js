describe('check index renders', () => {
  beforeEach(() => {
    cy.visit('/')
  })

  it('displays the title text', () => {
    cy.get('[data-cy="top-link"]').contains(/^Conky$/)
  })
})
describe('check config settings', () => {
  beforeEach(() => {
    cy.visit('/config_settings')
  })

  it('displays the title text', () => {
    cy.get('[data-cy="top-link"]').contains(/^Conky$/)
    cy.get('[data-cy="page-heading"]').contains(/^Configuration settings$/)
  })
})
describe('check variables', () => {
  beforeEach(() => {
    cy.visit('/variables')
  })

  it('displays the title text', () => {
    cy.get('[data-cy="top-link"]').contains(/^Conky$/)
    cy.get('[data-cy="page-heading"]').contains(/^Variables$/)
  })
  it('has anchor links and can focus on them', () => {
    cy.get('[data-anchor-name="acpiacadapter"]').click()
    cy.get('[data-anchor-name="acpiacadapter"]').and('be.visible')
  })
})
describe('check lua', () => {
  beforeEach(() => {
    cy.visit('/lua')
  })

  it('displays the title text', () => {
    cy.get('[data-cy="top-link"]').contains(/^Conky$/)
    cy.get('[data-cy="page-heading"]').contains(/^Lua API$/)
  })
})

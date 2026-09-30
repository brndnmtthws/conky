describe('documentation search navigation', () => {
  function selectSetting(name) {
    cy.get('button[title="Search (/ or ⌘K)"]').click()
    cy.get('input[placeholder="Search docs (/ or ⌘K)"]').clear().type(name)
    cy.get('[role="option"]').contains('code', new RegExp(`^${name}$`)).click()
    cy.location('hash').should('eq', `#${name}`)
  }

  function assertTargetInViewport(name) {
    cy.get(`#${name}`).should(($entry) => {
      expect($entry).to.have.attr('data-target')
      const { top, bottom } = $entry[0].getBoundingClientRect()
      const viewportHeight = $entry[0].ownerDocument.defaultView.innerHeight
      expect(top).to.be.at.least(0)
      expect(top).to.be.lessThan(viewportHeight)
      expect(bottom).to.be.greaterThan(0)
    })
  }

  beforeEach(() => {
    cy.viewport(1280, 900)
    cy.visit('/config_settings')
  })

  it('scrolls back when search selects the same entry again', () => {
    selectSetting('output_backend')
    assertTargetInViewport('output_backend')

    cy.scrollTo('top')
    cy.get('#output_backend').should(($entry) => {
      const viewportHeight = $entry[0].ownerDocument.defaultView.innerHeight
      expect($entry[0].getBoundingClientRect().top).to.be.greaterThan(viewportHeight)
    })

    selectSetting('output_backend')
    assertTargetInViewport('output_backend')
    cy.get('[data-target]').should('have.length', 1)

    selectSetting('update_interval')
    assertTargetInViewport('update_interval')
    cy.get('#output_backend').should('not.have.attr', 'data-target')
    cy.get('[data-target]').should('have.length', 1)
  })
})

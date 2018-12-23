<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet 
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform" 
  xmlns:exsl="http://exslt.org/common"
  exclude-result-prefixes="exsl"
  version="1.0">

<!-- $Id$ -->


<!-- Support for the DocBook Publishers extension -->
<xsl:template match="drama">

  <div>
    <xsl:call-template name="common.html.attributes"/>
    <xsl:call-template name="id.attribute"/>
    <xsl:call-template name="anchor"/>

    <xsl:call-template name="drama.titlepage"/>

    <xsl:apply-templates/>

  </div>
</xsl:template>

<xsl:template match="stagedir">

  <div>
    <xsl:call-template name="common.html.attributes"/>
    <xsl:call-template name="id.attribute"/>
    <xsl:choose>
      <xsl:when test="$make.clean.html = 0">
        <xsl:attribute name="style">
          <xsl:text>font-style:italic; font-weight:bold;</xsl:text>
        </xsl:attribute>
      </xsl:when>
      <xsl:otherwise>
      </xsl:otherwise>
    </xsl:choose>

    <xsl:call-template name="anchor"/>

    <xsl:apply-templates/>

  </div>
</xsl:template>

<xsl:template match="inlinestagedir">

  <span>
    <xsl:call-template name="common.html.attributes"/>
    <xsl:call-template name="id.attribute"/>
    <xsl:choose>
      <xsl:when test="$make.clean.html = 0">
        <xsl:attribute name="style">
          <xsl:text>font-style:italic; font-weight:bold;</xsl:text>
        </xsl:attribute>
        <xsl:call-template name="anchor"/>
        <xsl:text> [</xsl:text>
        <xsl:apply-templates/>
        <xsl:text>] </xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="anchor"/>
        <xsl:apply-templates/>
      </xsl:otherwise>
    </xsl:choose>
  </span>
</xsl:template>

<xsl:template match="linegroup">
  <div>
    <xsl:call-template name="common.html.attributes"/>
    <xsl:call-template name="id.attribute"/>
    <xsl:if test="$make.clean.html = 0">
      <xsl:attribute name="style">
        <xsl:text>width: 100%; display: table; margin-top: 5px;</xsl:text>
      </xsl:attribute>
    </xsl:if>
    <xsl:call-template name="anchor"/>

    <div>
      <xsl:if test="$make.clean.html = 0">
        <xsl:attribute name="style">display: table-row;</xsl:attribute>
      </xsl:if>
      <div>
        <xsl:if test="$make.clean.html = 0">
          <xsl:attribute name="style">display: table-cell; width: 15%</xsl:attribute>
        </xsl:if>
        <xsl:apply-templates select="speaker"/>
      </div>

      <div>
        <xsl:if test="$make.clean.html = 0">
          <xsl:attribute name="style">display: table-cell; width: 85%</xsl:attribute>
        </xsl:if>
        <xsl:apply-templates select="*[not(self::speaker)]"/>
      </div>

    </div>

  </div>
</xsl:template>

<xsl:template match="speaker">
  <div>
    <xsl:call-template name="common.html.attributes"/>
    <xsl:call-template name="id.attribute"/>
    <xsl:call-template name="anchor"/>
    <xsl:apply-templates/>
  </div>
</xsl:template>

<xsl:template match="line">
  <div>
    <xsl:call-template name="common.html.attributes"/>
    <xsl:call-template name="id.attribute"/>
    <xsl:call-template name="anchor"/>
    <xsl:apply-templates/>
  </div>
</xsl:template>

<xsl:template match="drama/title"/>
<xsl:template match="poetry/title"/>
<xsl:template match="dialogue/title"/>

</xsl:stylesheet>

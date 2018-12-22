<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
		xmlns:its="http://www.w3.org/2005/11/its"
		exclude-result-prefixes="its"
                version='1.0'>

<!-- ********************************************************************
     $Id: html.xsl 9306 2012-04-28 03:49:00Z bobstayton $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://docbook.sf.net/release/xsl/current/ for
     copyright and other information.

     Templates in this stylesheet convert ITS 2.0 markup
     http://www.w3.org/TR/its20/ into corresponding HTML5 attributes
     (prefixed with its-*).

     ******************************************************************** -->

<!-- List of recognized ITS attributes -->
<xsl:variable name="its-attrs"> its-allowed-characters its-annotators-ref its-line-break-type its-loc-note its-loc-note-ref its-loc-note-type its-loc-quality-issue-comment its-loc-quality-issue-enabled its-loc-quality-issue-profile-ref its-loc-quality-issue-severity its-loc-quality-issue-type its-loc-quality-issues-ref its-loc-quality-rating-profile-ref its-loc-quality-rating-score its-loc-quality-rating-score-threshold its-loc-quality-rating-vote its-loc-quality-rating-vote-threshold its-locale-filter-list its-locale-filter-type its-mt-confidence its-org its-org-ref its-person its-person-ref its-prov-ref its-provenance-records-ref its-rev-org its-rev-org-ref its-rev-person its-rev-person-ref its-rev-tool its-rev-tool-ref its-storage-encoding its-storage-size its-ta-class-ref its-ta-confidence its-ta-ident its-ta-ident-ref its-ta-source its-term its-term-confidence its-term-info-ref its-tool its-tool-ref its-within-text </xsl:variable>

<xsl:template name="its.attributes">
  <xsl:param name="inherit" select="0"/>
  <xsl:apply-templates select="." mode="its.attributes">
    <xsl:with-param name="inherit" select="$inherit"/>
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="*" mode="its.attributes">
  <xsl:param name="inherit" select="0"/>

  <xsl:choose>
    <!-- Handle inheritance; especially necessary for chunking -->
    <xsl:when test="$inherit = 1">
      <xsl:variable name="attrs" select="ancestor-or-self::*/@*[namespace-uri() = 'http://www.w3.org/2005/11/its']"/>
      <xsl:for-each select="$attrs">
	<xsl:variable name="name" select="local-name(.)"/>
	<xsl:if test="not(..//*/@*[local-name(.) = $name and (count(. | $attrs) = 1)])">
	  <xsl:apply-templates select="."/>
	</xsl:if>
      </xsl:for-each>
    </xsl:when>
    <xsl:otherwise>
      <xsl:apply-templates select="@*[namespace-uri() = 'http://www.w3.org/2005/11/its']"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- translate attribute is special in HTML -->
<xsl:template match="@its:translate">
  <xsl:attribute name="translate">
    <xsl:value-of select="."/>
  </xsl:attribute>
</xsl:template>

<xsl:template match="@its:*">
  <xsl:variable name="attr">
    <xsl:call-template name="its-html-attribute-name">
      <xsl:with-param name="name" select="local-name(.)"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="contains($its-attrs, concat(' ', $attr, ' '))">
      <xsl:attribute name="{$attr}">
	<xsl:value-of select="."/>
      </xsl:attribute>
    </xsl:when>
    <xsl:otherwise>
      <xsl:message>Attribute <xsl:value-of select="name(.)"/> is not recognized as ITS attribute. Ignoring.</xsl:message>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="its-html-attribute-name">
  <xsl:param name="name"/>

  <xsl:text>its-</xsl:text>
  <xsl:call-template name="its-camel-case-to-dashes">
    <xsl:with-param name="text" select="$name"/>
  </xsl:call-template>
</xsl:template>

<xsl:template name="its-camel-case-to-dashes">
  <xsl:param name="text"/>

  <xsl:variable name="first" select="substring($text, 1, 1)"/>
  <xsl:variable name="rest" select="substring($text, 2)"/>

  <xsl:choose>
    <xsl:when test="$first != translate($first, 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz')">
      <xsl:value-of select="'-'"/>
      <xsl:value-of select="translate($first, 'ABCDEFGHIJKLMNOPQRSTUVWXYZ', 'abcdefghijklmnopqrstuvwxyz')"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$first"/>
    </xsl:otherwise>
  </xsl:choose>
  
  <xsl:if test="$rest != ''">
  <xsl:call-template name="its-camel-case-to-dashes">
      <xsl:with-param name="text" select="$rest"/>
    </xsl:call-template>
  </xsl:if>
</xsl:template>

</xsl:stylesheet>

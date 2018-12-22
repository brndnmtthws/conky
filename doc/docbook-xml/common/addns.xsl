<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:exsl="http://exslt.org/common"
                xmlns:fo="http://www.w3.org/1999/XSL/Format"
                xmlns:xlink="http://www.w3.org/1999/xlink"
                xmlns:d="http://docbook.org/ns/docbook"
                xmlns:saxon="http://icl.com/saxon"
                xmlns:NodeInfo="http://org.apache.xalan.lib.NodeInfo"
                exclude-result-prefixes="d exsl saxon NodeInfo"
                version='1.0'>

<xsl:import href="../common/utility.xsl"/>

<!-- Template to add the namespace to non-namespaced documents -->
<xsl:template match="*" mode="addNS">
  <xsl:element name="{local-name()}" 
          namespace="http://docbook.org/ns/docbook">
    <xsl:copy-of select="@*"/>

    <!-- Add xml:base so relative paths don't get lost -->
    <xsl:if test="not(../..)">
      <xsl:call-template name="add-xml-base"/>
    </xsl:if>

    <xsl:apply-templates select="node()" mode="addNS"/>
  </xsl:element>
</xsl:template>

<xsl:template match="processing-instruction()|comment()" mode="addNS">
  <xsl:copy/>
</xsl:template>

<xsl:template name="add-xml-base">
  <!-- * Get a title for current doc so that we let the user -->
  <!-- * know what document we are processing at this point. -->
  <xsl:variable name="doc.title">
    <xsl:call-template name="get.doc.title"/>
  </xsl:variable>
  <xsl:if test="not(@xml:base)">
    <xsl:variable name="base">
      <xsl:choose>
        <xsl:when test="function-available('saxon:systemId')">
          <xsl:value-of select="saxon:systemId()"/>
        </xsl:when>
        <xsl:when test="function-available('NodeInfo:systemId')">
          <xsl:value-of select="NodeInfo:systemId()"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="log.message">
            <xsl:with-param name="level">Warn</xsl:with-param>
            <xsl:with-param name="source" select="$doc.title"/>
            <xsl:with-param name="context-desc">
              <xsl:text>no @xml:base</xsl:text>
            </xsl:with-param>
            <xsl:with-param name="message">
              <xsl:text>cannot add @xml:base to node-set root element</xsl:text>
            </xsl:with-param>
          </xsl:call-template>
          <xsl:call-template name="log.message">
            <xsl:with-param name="level">Warn</xsl:with-param>
            <xsl:with-param name="source" select="$doc.title"/>
            <xsl:with-param name="context-desc">
              <xsl:text>no @xml:base</xsl:text>
            </xsl:with-param>
            <xsl:with-param name="message">
              <xsl:text>relative paths may not work</xsl:text>
            </xsl:with-param>
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:variable>
    <!-- debug
    <xsl:message>base is <xsl:value-of select="$base"/></xsl:message>
    -->
    <xsl:if test="$base != ''">
      <xsl:attribute name="xml:base">
        <xsl:call-template name="systemIdToBaseURI">
          <xsl:with-param name="systemId">
            <!-- file: seems to confuse some processors. -->
            <xsl:choose>
              <!-- however, windows paths must use file:///c:/path -->
              <xsl:when test="starts-with($base, 'file:///') and
                              substring($base, 10, 1) = ':'">
                <xsl:value-of select="$base"/>
              </xsl:when>
              <xsl:when test="starts-with($base, 'file:/')
                              and substring($base, 8, 1) = ':'">
                <xsl:value-of select="concat('file:///', 
                                      substring-after($base,'file:/'))"/>
              </xsl:when>
              <xsl:when test="starts-with($base, 'file:///')">
                <xsl:value-of select="substring-after($base,'file://')"/>
              </xsl:when>
              <xsl:when test="starts-with($base, 'file://')">
                <xsl:value-of select="substring-after($base,'file:/')"/>
              </xsl:when>
              <xsl:when test="starts-with($base, 'file:/')">
                <xsl:value-of select="substring-after($base,'file:')"/>
              </xsl:when>
              <xsl:otherwise>
                <xsl:value-of select="$base"/>
              </xsl:otherwise>
            </xsl:choose>
          </xsl:with-param>
        </xsl:call-template>
      </xsl:attribute>
    </xsl:if>
  </xsl:if>
</xsl:template>

<xsl:template name="systemIdToBaseURI">
  <xsl:param name="systemId" select="''"/>
  <xsl:if test="contains($systemId,'/')">
    <xsl:value-of select="substring-before($systemId,'/')"/>
    <xsl:text>/</xsl:text>
    <xsl:call-template name="systemIdToBaseURI">
      <xsl:with-param name="systemId"
                      select="substring-after($systemId,'/')"/>
    </xsl:call-template>
  </xsl:if>
</xsl:template>

</xsl:stylesheet>


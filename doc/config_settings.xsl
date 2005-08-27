<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  version="1.0">
  
  <!--jtan325 created this-->  
  <xsl:output method="html"/>
  
  <xsl:template match="/">
    <html>
      <head>
        <title>Conky Variables</title>
      </head>
      <body bgcolor="#FFFFFF">
        <xsl:apply-templates />
      </body>
    </html>
  </xsl:template>

  <xsl:template match="variablelist" >
    <table cellpadding="4">

      <tr bgcolor = "#ffd700">
        <th>Option</th>
        <th>Explanation</th>
      </tr>

      <xsl:for-each select="varlistentry">
        <tr bgcolor = "#4a708b">
          <td align="center" bgcolor="#2e8b57">
            <font color = "FFFFFF">
              <xsl:value-of select="term/command/option" />
            </font>              
          </td>
          <td>
            <font color = "#c1cdcd"> 
              <xsl:value-of select="listitem" />
            </font>
          </td>
        </tr>
      </xsl:for-each>
      
    </table>
  </xsl:template>
  
</xsl:stylesheet>
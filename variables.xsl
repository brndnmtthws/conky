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

  <xsl:template match="variables" >
    <table cellpadding="4">

      <tr bgcolor = "#ffd700">
        <th>Variable</th>
        <th>Arguments () = optional</th>
        <th>Explanation</th>
        <th>Example</th>
      </tr>

      <xsl:for-each select="variable">
        <tr bgcolor = "#4a708b">
          <td align="center" bgcolor="#2e8b57">
            <font color = "FFFFFF">
              <xsl:value-of select="name" />
            </font>
          </td>
          <td align="center">
            <xsl:value-of select="arguments" />
          </td>
          <td>
            <xsl:value-of select="explanation" />
          </td>
          <td>
            <xsl:value-of select="example" />
          </td>
        </tr>
      </xsl:for-each>

    </table>
  </xsl:template>

</xsl:stylesheet>

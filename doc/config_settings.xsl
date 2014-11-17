<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	version="1.0">

	<xsl:output method="html"/>

	<xsl:template match="/">
		<html>
			<head>
				<title>~/.config/conky/conky.conf settings</title>
			</head>
			<body bgcolor="#FFFFFF">
				<xsl:apply-templates />
			</body>
		</html>
	</xsl:template>

	<xsl:template match="/">
		<html>
			<head>
				<title>Conky configuration settings</title>
			</head>
			<body bgcolor="#FFFFFF">
				<xsl:apply-templates />
			</body>
		</html>
	</xsl:template>

	<xsl:template match="/">
		<html>
			<head>
				<title>Configuration Settings</title>
			</head>
			<body bgcolor="#FFFFFF">
				<xsl:apply-templates />
			</body>
		</html>
	</xsl:template>

	<xsl:template match="member/command">
		<strong>
			<xsl:value-of select="." />
		</strong> - 
	</xsl:template>

	<xsl:template match="member/option">
		<xsl:value-of select="." />
	</xsl:template>

	<xsl:template match="member">
		<li>
			<xsl:apply-templates />
		</li>
	</xsl:template>

	<xsl:template match="simplelist">
		<ul>
			<xsl:apply-templates select="member" />
		</ul>
	</xsl:template>

	<xsl:template match="para">
		<p>
			<xsl:value-of select="." />
		</p>
	</xsl:template>

	<xsl:template match="variablelist" >
		<table cellpadding="3">

			<tr bgcolor = "#eecfa1">
				<th>Variable</th>
				<th>Explanation</th>
			</tr>

			<xsl:for-each select="varlistentry">
				<xsl:variable name="row_bg">
					<xsl:choose>
						<xsl:when test="position() mod 2 = 1">#fffafa</xsl:when>
						<xsl:otherwise>#b4cdcd</xsl:otherwise>
					</xsl:choose>
				</xsl:variable>
				<tr bgcolor = "{$row_bg}">
					<td align="center">
						<xsl:value-of select="term/command/option" />
					</td>
					<td>
						<xsl:apply-templates select="listitem" />
					</td>
				</tr>
			</xsl:for-each>
		</table>
	</xsl:template>

</xsl:stylesheet>

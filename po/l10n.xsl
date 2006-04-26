<?xml version='1.0' ?>
<xsl:stylesheet version='2.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>
	<xsl:template match='/project'>
		<html>
			<head>
				<title><xsl:value-of select='@name'/> translation statistics</title>
				<!-- <link rel="Stylesheet" href="/gaim.css" type="text/css" media="screen" /> -->
				<style>
					.bargraph {
						width: 200px;
						height: 20px;
						background: black;
						border-collapse: collapse;
						border-spacing: 0px;
						margin: 0px;
						border: 0px;
						padding: 0px;
					}

				</style>
			</head>
			<body>
				<!-- <div id="content"> -->
				<h1>aMule translation statistics (generated daily)</h1>
				<table>
					<tr><th>Language  </th><th colspan='2'>Translated</th><th colspan='3'>Fuzzy</th><th colspan='3'>Untranslated</th></tr>
					<xsl:for-each select="lang">
						<xsl:sort select='@translated' data-type="number" order="descending"/>
						<tr>
							<td><a><xsl:attribute name='href'>files/<xsl:value-of select='@code'/>.po</xsl:attribute><xsl:value-of select='@name'/> (<xsl:value-of select='@code'/>)</a></td>
							<td>| <xsl:value-of select='@translated'/></td><td> ~ <xsl:value-of select="format-number(@translated div ../@strings * 100,'#.##')"/>%</td>
							<td>| | <xsl:value-of select='@fuzzy'/></td> ~ <td><xsl:value-of select="format-number(@fuzzy div ../@strings * 100,'#.##')"/>%</td>
							<td>| | <xsl:value-of select='../@strings - (@translated + @fuzzy)'/></td> ~ <td><xsl:value-of select="format-number((../@strings - (@translated + @fuzzy)) div ../@strings * 100,'#.##')"/>%</td>
						<td>
							<table class='bargraph'><tr>
									<td bgcolor='green'><xsl:attribute name='width'><xsl:value-of select='round(@translated div ../@strings * 200)'/>px;</xsl:attribute></td>
									<td bgcolor='blue'><xsl:attribute name='width'><xsl:value-of select='round(@fuzzy div ../@strings * 200)'/>px;</xsl:attribute></td>
									<!-- <td bgcolor='red'><xsl:attribute name='width'><xsl:value-of select='200 - round((@translated + @fuzzy) div ../@strings * 200)'/>px;</xsl:attribute></td> -->
									<td bgcolor='red'></td>
							</tr></table>
						</td>
						</tr>
					</xsl:for-each>
				</table>
				<a><xsl:attribute name='href'><xsl:value-of select='@pofile'/></xsl:attribute><xsl:value-of select='@pofile'/></a> generated on <xsl:value-of select='@generated'/>
				<!-- </div> -->
			</body>
		</html>
	</xsl:template>
</xsl:stylesheet>

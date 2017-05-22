<?xml version='1.0' ?>
<xsl:stylesheet version='2.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>
	<xsl:template match='/project'>
		<html>
			<head>
				<title><xsl:value-of select='@name'/> translation statistics</title>
				<!-- <link rel="Stylesheet" href="../style.css" type="text/css" media="screen" /> -->
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
				<h1><xsl:value-of select='@name'/> translation statistics</h1>
				Statistics generated <xsl:value-of select='@stats_generated' /><br/><br/>
				<table>
					<tr><th>Language  </th><th /><th colspan='2'>Translated</th><th /><th colspan='2'>Fuzzy</th><th /><th colspan='2'>Untranslated</th><th colspan='3'>Warnings</th><th>Progress</th></tr>
					<xsl:for-each select="lang">
						<xsl:sort select='@translated' data-type="number" order="descending"/>
						<tr>
							<td><a><xsl:attribute name='href'><xsl:value-of select='@code'/>.po</xsl:attribute><xsl:value-of select='@name'/> (<xsl:value-of select='@code'/>)</a></td>
							<td>|</td>
							<td><xsl:value-of select='@translated'/></td><td> ~ <xsl:value-of select="format-number(@translated div ../@strings * 100,'#.##')"/>%</td>
							<td>| |</td>
							<td><xsl:value-of select='@fuzzy'/></td><td> ~ <xsl:value-of select="format-number(@fuzzy div ../@strings * 100,'#.##')"/>%</td>
							<td>| |</td>
							<td><xsl:value-of select='../@strings - (@translated + @fuzzy)'/></td><td> ~ <xsl:value-of select="format-number((../@strings - (@translated + @fuzzy)) div ../@strings * 100,'#.##')"/>%</td>
							<td>| |</td>
							<xsl:choose>
								<xsl:when test="@warnings = 0">
									<td><xsl:value-of select='@warnings'/></td>
								</xsl:when>
								<xsl:otherwise>
									<td bgcolor="#ff0000">
									<a><xsl:attribute name='href'><xsl:value-of select='@code'/>.po.warnings</xsl:attribute><xsl:value-of select='@warnings'/></a>
									</td>
								</xsl:otherwise>
							</xsl:choose>
							<td>|</td>
						<td>
							<table class='bargraph'><tr>
								<xsl:if test="@translated > 0">
									<td bgcolor='green'><xsl:attribute name='width'><xsl:value-of select='round(@translated div ../@strings * 200)'/>px;</xsl:attribute></td>
								</xsl:if>
								<xsl:if test="@fuzzy > 0">
									<xsl:choose>
										<xsl:when test="../@strings - @translated - @fuzzy > 0">
											<td bgcolor='blue'><xsl:attribute name='width'><xsl:value-of select='round(@fuzzy div ../@strings * 200)'/>px;</xsl:attribute></td>
										</xsl:when>
										<xsl:otherwise>
											<td bgcolor='blue'><xsl:attribute name='width'><xsl:value-of select='200 - round(@translated div ../@strings * 200)'/>px;</xsl:attribute></td>
										</xsl:otherwise>
									</xsl:choose>
								</xsl:if>
								<xsl:if test="../@strings - @translated - @fuzzy > 0">
									<td bgcolor='red'><xsl:attribute name='width'><xsl:value-of select='200 - round(@translated div ../@strings * 200) - round(@fuzzy div ../@strings * 200)'/>px;</xsl:attribute></td>
									<!-- <td bgcolor='red'></td> -->
								</xsl:if>
							</tr></table>
						</td>
						</tr>
					</xsl:for-each>
				</table>
				<br/>
				<a><xsl:attribute name='href'><xsl:value-of select='@pofile'/></xsl:attribute><xsl:value-of select='@pofile'/></a> generated on <xsl:value-of select='@generated'/>
				<!-- </div> -->
			</body>
		</html>
	</xsl:template>
</xsl:stylesheet>

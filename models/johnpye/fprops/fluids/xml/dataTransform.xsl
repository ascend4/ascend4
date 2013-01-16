<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
    
    <xsl:template match="/fluid">
        <html>
            <head>
                <style type="text/css" media="all">@import "style.css";</style>
            </head>
            <body>
            <div id="page-container">
                <div class="title">
                    <img src="ascend-logo.png" alt="Ascend logo" width="50px" /><h1>ASCEND: FPROPS</h1>
                    <h2><xsl:value-of select="name"/></h2>
                </div>
                <div class="data">
                    <xsl:for-each select="core">
                        <h3>Core Data</h3>
                        <p class="subtle">
                            (Source: <xsl:value-of select="@source" />, Rank: <xsl:value-of select="@rank" />)
                        </p>
                        <table>
                            <tr>
                                <th>Property</th><th>Value</th>
                            </tr>
                            <tr>
                                <td>Molar Mass</td><td><xsl:value-of select="M" /></td>
                            </tr>
                            <tr class="alt">
                                <td>Critical Temperature</td><td><xsl:value-of select="T_c" /></td>
                            </tr>
                            <tr>
                                <td>Critical Pressure</td><td><xsl:value-of select="P_c" /></td>
                            </tr>
                            <tr class="alt">
                                <td>Critical Density</td><td><xsl:value-of select="rho_c" /></td>
                            </tr>
                            <tr>
                                <td>Acentric Factor</td><td><xsl:value-of select="omega" /></td>
                            </tr>
                            <tr class="alt">
                                <td>Triple Point Temperature</td><td><xsl:value-of select="T_t" /></td>
                            </tr>
                        </table>
                    </xsl:for-each>
                    <xsl:for-each select="ideal">
                        <h3>Ideal Gas Data</h3>
                        <p class="subtle">
                            (Source: <xsl:value-of select="@source" />, Rank: <xsl:value-of select="@rank" />)
                        </p>
                        <table>
                            <tr>
                                <th>Property</th><th>Value</th>
                            </tr>
                            <tr>
                                <td>Constant Term</td><td><xsl:value-of select="const" /></td>
                            </tr>
                            <tr class="alt">
                                <td>Linear Term</td><td><xsl:value-of select="lin" /></td>
                            </tr>
                            <tr>
                                <td>Reference Heat Capacity</td><td><xsl:value-of select="Cp_star" /></td>
                            </tr>
                            <tr class="alt">
                                <td>Power Terms</td><td><xsl:value-of select="powerTerms/@number" /></td>
                            </tr>
                        </table>
                        <table class="terms">
                            <tr>
                                <th>c</th><th>t</th>
                            </tr>
                            <xsl:for-each select="powerTerms/term">
                                <tr>
                                    <td><xsl:value-of select="c" /></td>
                                    <td><xsl:value-of select="t" /></td>
                                </tr>
                            </xsl:for-each>
                        </table>
                    </xsl:for-each>
                    <xsl:for-each select="helmholtz">
                        <h3>Helmholtz</h3>
                        <p class="subtle">
                            (Source: <xsl:value-of select="@source" />, Rank: <xsl:value-of select="@rank" />)
                        </p>
                        <table>
                            <tr>
                                <th>Property</th><th>Value</th>
                            </tr>
                            <tr>
                                <td>Power Terms</td><td><xsl:value-of select="powerTerms/@number" /></td>
                            </tr>
                        </table>
                        <table class="terms">
                            <tr>
                                <th>a</th><th>d</th><th>t</th><th>l</th>
                            </tr>
                            <xsl:for-each select="powerTerms/term">
                                <tr>
                                    <td><xsl:value-of select="a" /></td>
                                    <td><xsl:value-of select="d" /></td>
                                    <td><xsl:value-of select="t" /></td>
                                    <td><xsl:value-of select="l" /></td>
                                </tr>
                            </xsl:for-each>
                        </table>
                    </xsl:for-each>
                </div>
            </div>
            </body>
        </html>
    </xsl:template>
    
</xsl:stylesheet>
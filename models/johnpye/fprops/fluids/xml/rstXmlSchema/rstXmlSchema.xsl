<?xml version="1.0" encoding="ISO-8859-1"?>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
	<xsl:output method="html" version="1.0" encoding="UTF-8" indent="yes" omit-xml-declaration="no" doctype-system="about:legacy-compat"/>
	<xsl:template match="/" name="root">
		<html>
			<head>
				<style type="text/css" media="all">@import "rstXmlSchema.css";</style>
				<script type="text/javascript" src="http://ajax.googleapis.com/ajax/libs/jquery/1.7.1/jquery.min.js"></script>
				<script type="text/javascript" src="http://ajax.googleapis.com/ajax/libs/jqueryui/1.8.13/jquery-ui.min.js"></script>
				<script type="text/javascript" src="jquery.jsPlumb-1.3.7-all.js"></script>
				<script type="text/javascript" src="rstXmlSchema.js"></script>
			</head>
			<body onunload="jsPlumb.unload();">
				<form class="settings" onkeypress="return handleEnterKeypress()">
					<div><label target="lines">Show connectors (this can be a bit slow): </label><input type="checkbox" name="lines" id="lines" checked="checked" onclick="changeSettings()"/></div>
					<div><label target="boxes">Show boxes: </label><input type="checkbox" name="boxes" id="boxes" onclick="changeSettings()" /></div>
					<div><label target="width">Width (% screen size): </label><input type="number" name="width" id="width" onchange="changeWidth()" value="100"/></div>
					<div><label target="detail">Detail: </label><input type="range" name="detail" id="detail" value="100" onchange="changeDetail()"/></div>
				</form>
				<div class="container">
				<xsl:for-each select="*[local-name()='schema']">
					<xsl:for-each select="*[local-name()='element']">
						<xsl:call-template name="element"/>
					</xsl:for-each>
				</xsl:for-each>
				</div>
			</body>
		</html>
	</xsl:template>
	<xsl:template match="*" name="element">
		<!--Create the element and all of its children.-->
		<div class="branch">
			<div class="elementContainer">
				<div class="element">
					<xsl:if test="@minOccurs = '0'">
						<xsl:attribute name="class">element optional</xsl:attribute>
					</xsl:if>
					<xsl:value-of select="@name"/>
					<xsl:if test="count(.//*[local-name()='element']) > 0 or count(key('typeKey',@type)//*[local-name()='element']) > 0">
						<img class="show" src="plus.gif" onclick="showChildren(this)" />
						<img class="hide" src="minus.gif" onclick="hideChildren(this)" style="display:none;"/>
					</xsl:if>
				</div>
				<span class="annotation">
					<xsl:for-each select="*/*[local-name()='documentation']">
						<xsl:value-of select="."/>
					</xsl:for-each>
				</span>
			</div>
			<!--Create those that are directly children:-->
			<xsl:for-each select="*">
				<xsl:call-template name="allSequenceChoice"/>
			</xsl:for-each>
			<!--If the element has a 'type' then create all children for its type as well-->
			<xsl:call-template name="type"/>
		</div>
		<!--div class="clear"/-->
	</xsl:template>
	<xsl:template match="*" name="allSequenceChoice">
		<xsl:for-each select="(./*[local-name()='sequence'])">
			<xsl:call-template name="sequence"/>
		</xsl:for-each>
		<xsl:for-each select="(./*[local-name()='all'])">
			<xsl:call-template name="all"/>
		</xsl:for-each>
		<xsl:for-each select="(./*[local-name()='choice'])">
			<xsl:call-template name="choice"/>
		</xsl:for-each>
	</xsl:template>
	<xsl:template match="*" name="sequence">
		<div class="branch">
			<div class="modifierContainer">
				<div class="modifier" title="xs:sequence: You must provide all of the mandatory children in sequence.">
					xs:sequence
					<xsl:if test="count(.//*[local-name()='element']) > 0 or count(key('typeKey',@type)//*[local-name()='element']) > 0">
						<img class="show" src="plus.gif" onclick="showChildren(this)" />
						<img class="hide" src="minus.gif" onclick="hideChildren(this)" style="display:none;"/>
					</xsl:if>
				</div>
				<span class="annotation">
					<xsl:for-each select="*/*[local-name()='documentation']">
						<xsl:value-of select="."/>
					</xsl:for-each>
				</span>
			</div>
			<!--Create those that are directly children:-->
			<xsl:for-each select="(./*[local-name()='element'])">
				<xsl:call-template name="element"/>
			</xsl:for-each>
		</div>
	</xsl:template>
	<xsl:template match="*" name="all">
		<div class="branch">
			<div class="modifierContainer">
				<div class="modifier" title="xs:all: You must provide all of the mandatory children in any order.">
					xs:all
					<xsl:if test="count(.//*[local-name()='element']) > 0 or count(key('typeKey',@type)//*[local-name()='element']) > 0">
						<img class="show" src="plus.gif" onclick="showChildren(this)" />
						<img class="hide" src="minus.gif" onclick="hideChildren(this)" style="display:none;"/>
					</xsl:if>
				</div>
				<span class="annotation">
					<xsl:for-each select="*/*[local-name()='documentation']">
						<xsl:value-of select="."/>
					</xsl:for-each>
				</span>
			</div>
			<!--Create those that are directly children:-->
			<xsl:for-each select="(./*[local-name()='element'])">
				<xsl:call-template name="element"/>
			</xsl:for-each>
		</div>
	</xsl:template>
	<xsl:template match="*" name="choice">
		<div class="branch">
			<div class="modifierContainer" title="xs:choice: You must provide exactly one of the children.">
				<div class="modifier">
					xs:choice
					<xsl:if test="count(.//*[local-name()='element']) > 0 or count(key('typeKey',@type)//*[local-name()='element']) > 0">
						<img class="show" src="plus.gif" onclick="showChildren(this)" />
						<img class="hide" src="minus.gif" onclick="hideChildren(this)" style="display:none;"/>
					</xsl:if>
				</div>
				<span class="annotation">
					<xsl:for-each select="*/*[local-name()='documentation']">
						<xsl:value-of select="."/>
					</xsl:for-each>
				</span>
			</div>
			<!--Create those that are directly children:-->
			<xsl:for-each select="(./*[local-name()='element'])">
				<xsl:call-template name="element"/>
			</xsl:for-each>
		</div>
	</xsl:template>
	<xsl:key name="typeKey" match="//*[local-name()='complexType']" use="@name"/>
	<xsl:template match="@type" name="type">
		<xsl:for-each select="key('typeKey',@type)">
			<xsl:call-template name="allSequenceChoice"/>
			<xsl:for-each select="./*[local-name()='complexContent']">
				<xsl:call-template name="allSequenceChoice"/>
				<xsl:for-each select="./*[local-name()='extension']">
					<xsl:call-template name="allSequenceChoice"/>
				</xsl:for-each>
			</xsl:for-each>
		</xsl:for-each>
	</xsl:template>
</xsl:stylesheet>
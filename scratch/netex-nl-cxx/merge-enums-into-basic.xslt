<?xml version="1.0" encoding="UTF-8"?>
<xsl:transform version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xsd="http://www.w3.org/2001/XMLSchema">
  <!-- Replace includes of netex-nl-enums.xsd with the (almost) literal
       contents of the xsd:schema definition in netex-nl-enums.xsd -->
  <xsl:template match="xsd:include[@schemaLocation='netex-nl-enums.xsd']">
    <xsl:apply-templates select="document('netex-nl-enums.xsd', /)/xsd:schema/node()"/>
  </xsl:template>

  <!-- Remove the big 'AFHANKELIJKHEDEN' (dependencies) comment, which
       otherwise looks out of place in the output -->
  <xsl:template match="comment()[following-sibling::*[1]/self::xsd:include[@schemaLocation='netex-nl-basic.xsd']]"/>

  <!-- Remove includes of netex-nl-basic.xsd (coming from netex-nl-enums.xsd),
       because we basically inline netex-nl-enums.xsd into netex-nl-basic.xsd -->
  <xsl:template match="xsd:include[@schemaLocation='netex-nl-basic.xsd']"/>

  <!-- Apply the transformation on the entire tree -->
  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()"/>
    </xsl:copy>
  </xsl:template>
</xsl:transform>

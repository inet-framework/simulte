/**
 * Copyright (c) 2012 Opensim Ltd. All rights reserved.
 */

package com.simulte.configeditor.propertysources;

import org.eclipse.core.runtime.Assert;
import org.omnetpp.common.ui.HoverSupport;
import org.omnetpp.common.util.StringUtils;
import org.w3c.dom.Attr;

/**
 * A property source that represents an attribute in a DOM tree.
 * 
 * @author Andras
 */
public class XmlAttributePropertySource implements IPropertySource {
    protected ParsedXmlIniParameter xmlIniParameter;
    protected IXmlAttributeProvider attrProvider;
    protected Attr attrNode;
    protected String defaultValue;

    public XmlAttributePropertySource(ParsedXmlIniParameter xmlIniParameter, IXmlAttributeProvider attrProvider, String defaultValue) {
        this.xmlIniParameter = xmlIniParameter;
        this.attrProvider = attrProvider;
        this.defaultValue = defaultValue;
    }

    public void initialize() {
        attrNode = attrProvider.getEditedAttr(xmlIniParameter.getXmlDocument());
        Assert.isNotNull(attrNode);
    }

    @Override
    public String getValueForEditing() {
        String value = attrNode.getValue();
        if (StringUtils.isBlank(value))
            value = defaultValue;
        return value;
    }

    @Override
    public void setValue(String value) {
        attrNode.setValue(value);
    }

    @Override
    public String getHoverText() {
        String text = "";
        
        IniParameterPropertySource propertySource = (IniParameterPropertySource)xmlIniParameter.getPropertySource();
        text += "<b>" + propertySource.getIniKey() + " = xml(" + StringUtils.quoteForHtml(attrProvider.getAttrPathInfo()) + ")" + "</b>";
        if (StringUtils.isNotBlank(defaultValue))
            text += "<br>default: " + defaultValue;
       
        return HoverSupport.addHTMLStyleSheet(text);
    }
}

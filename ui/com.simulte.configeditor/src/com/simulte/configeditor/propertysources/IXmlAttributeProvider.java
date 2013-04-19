/**
 * Copyright (c) 2012 Opensim Ltd. All rights reserved.
 */

package com.simulte.configeditor.propertysources;

import org.w3c.dom.Attr;
import org.w3c.dom.Document;

/**
 * Interface for classes that return an attribute in a DOM tree. Used for
 * providing input for XmlAttributePropertySource.
 * 
 * @author Andras
 */
public interface IXmlAttributeProvider {
    /**
     * Returns the attribute.
     */
    Attr getEditedAttr(Document xmlDoc);

    /**
     * Returns the path of the attribute in the XML, for displaying to the user.
     * (Note: this method is needed because Attr.getParentNode() always return null.) 
     */
    String getAttrPathInfo();

}

/**
 * Copyright (c) 2012 Opensim Ltd. All rights reserved.
 */

package com.simulte.configeditor.propertysources;

import java.io.ByteArrayInputStream;
import java.io.StringWriter;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.OutputKeys;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

import org.omnetpp.common.engine.Common;
import org.omnetpp.common.util.StringUtils;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.w3c.dom.Text;

import com.simulte.configeditor.SimuLTEPlugin;

/**
 * Utility class that stored XML in its parsed form (DOM tree).
 * 
 * @author Andras
 */
public class ParsedXmlIniParameter {
    private IPropertySource propertySource; // provides the XML text
    private Document xmlDocument;  // the parsed XML (DOM tree)
    
    public ParsedXmlIniParameter(IPropertySource propertySource) {
        this.propertySource = propertySource;
    }

    public void parse() {
        propertySource.initialize();
        
        // parse XML
        String value = propertySource.getValueForEditing();
        value = StringUtils.defaultIfEmpty(value, "xml(\"<root/>\")");

        if (!value.matches("xml\\(.*\\)"))
            throw new RuntimeException("xml(...) expected");
        value = value.replaceFirst("xml\\((.*)\\)", "$1");
        try {
            value = Common.parseQuotedString(value);
            xmlDocument = parseXML(value);
        } 
        catch (Exception e) {
            //TODO error dialog or something
            SimuLTEPlugin.logError(e);
            xmlDocument = parseXML("<root/>"); // this must succeed
        }

    }

    public IPropertySource getPropertySource() {
        return propertySource;
    }
    
    public Document getXmlDocument() {
        return xmlDocument;
    }
    
    protected Document parseXML(String text) {
        try {
            DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
            factory.setIgnoringComments(true);
            factory.setCoalescing(true);
            factory.setIgnoringElementContentWhitespace(true);
            DocumentBuilder docBuilder = factory.newDocumentBuilder();
            Document doc = docBuilder.parse(new ByteArrayInputStream(text.getBytes()));
            removeWhitespaceNodes(doc.getDocumentElement());  // otherwise indentation won't work
            return doc;
        }
        catch (Exception e) { // catches ParserConfigurationException, SAXException, IOException
            throw new RuntimeException("Cannot parse XML", e); //TODO details
        }
    }

    protected static void removeWhitespaceNodes(Element e) {
        NodeList children = e.getChildNodes();
        for (int i = children.getLength() - 1; i >= 0; i--) {
            Node child = children.item(i);
            if (child instanceof Text && ((Text) child).getData().trim().length() == 0) {
                e.removeChild(child);
            }
            else if (child instanceof Element) {
                removeWhitespaceNodes((Element) child);
            }
        }
    }

    public void unparse() {
        // serialize back the XML into the ini parameter
        String xmlString = serializeXML(xmlDocument);
        String value = "xml(" + StringUtils.quoteString(xmlString).replace("\\r", "").replace("\\n", " \\\n    ") + ")";
        propertySource.setValue(value);
    }
    
    protected String serializeXML(Document xmlDocument) {
        try {
            // serialize
            TransformerFactory factory = TransformerFactory.newInstance();
            factory.setAttribute("indent-number", new Integer(2));
            Transformer transformer = factory.newTransformer();
            transformer.setOutputProperty(OutputKeys.OMIT_XML_DECLARATION, "yes");
            transformer.setOutputProperty(OutputKeys.INDENT, "yes");
            StringWriter writer = new StringWriter();
            transformer.transform(new DOMSource(xmlDocument), new StreamResult(writer));
            String content = writer.toString();
            content = content.replace('"', '\'');
            return content; 
        }
        catch (Exception e) {  // catches: ParserConfigurationException,ClassCastException,ClassNotFoundException,InstantiationException,IllegalAccessException
            throw new RuntimeException("Cannot serialize XML: " + e.getMessage(), e);
        }
    }
}

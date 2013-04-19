/**
 * Copyright (c) 2012 Opensim Ltd. All rights reserved.
 */

package com.simulte.configeditor.propertysources;

import java.util.Set;

import org.eclipse.core.runtime.Assert;
import org.omnetpp.common.editor.text.NedCommentFormatter;
import org.omnetpp.common.engine.Common;
import org.omnetpp.common.ui.HoverSupport;
import org.omnetpp.common.util.StringUtils;
import org.omnetpp.ned.core.INedResources;
import org.omnetpp.ned.core.NedResourcesPlugin;
import org.omnetpp.ned.model.NedElement;
import org.omnetpp.ned.model.ex.ParamElementEx;
import org.omnetpp.ned.model.interfaces.INedTypeInfo;

import com.simulte.configeditor.editors.SimplifiedInifileDocument;

/**
 * A property source that represents a model parameter in an inifile. 
 * 
 * @author Andras
 */
public class IniParameterPropertySource implements IPropertySource {
    private SimplifiedInifileDocument document;
    private String iniKey;
    private String name;
    private String ownerNedType;
    private String type;
    private String unit;
    private String defaultValue;
    private String comment;

    public IniParameterPropertySource(SimplifiedInifileDocument document, String iniKey, String ownerNedType, String defaultValue) {
        this.document = document;
        this.iniKey = iniKey;
        this.ownerNedType = ownerNedType;
        this.name = StringUtils.substringAfterLast(this.iniKey, ".");
        this.defaultValue = defaultValue;
        Assert.isNotNull(ownerNedType);
    }

    @Override
    public void initialize() {
        INedResources nedResources = NedResourcesPlugin.getNedResources();
        Assert.isTrue(!nedResources.isLoadingInProgress(), "Should be called when NED files have been completely loaded");
            
        // look up the NED type that is supposed to contain such parameter
        Set<INedTypeInfo> possibleTypes = nedResources.getToplevelNedTypesFromAllProjects(ownerNedType);
        if (possibleTypes.isEmpty())
            throw new RuntimeException("NED type " + ownerNedType + " not found -- some required project (SimuLTE, INET) not open?");
        INedTypeInfo nedTypeInfo = possibleTypes.toArray(new INedTypeInfo[]{})[0];

        // look up parameter and fill in the rest of the fields
        ParamElementEx param = nedTypeInfo.getParamDeclarations().get(name);
        if (param == null)
            throw new RuntimeException("NED type " + ownerNedType + " has no parameter named " + name);
        type = NedElement.parTypeToString(param.getType());
        unit = param.getUnit();
        if (StringUtils.isEmpty(defaultValue))
            defaultValue = StringUtils.defaultString(param.getValue());
        comment = param.getComment();
    }

    @Override
    public String getValueForEditing() {
        String value = document.getValue(iniKey);
        if (StringUtils.isBlank(value))
            value = defaultValue;
        if (type != null && type.equals("string"))
            try {
                value = Common.parseQuotedString(value);
            } catch (Exception e) {
                // misquoted -- leave as it is
            }
        return value;
    }
    
    @Override
    public void setValue(String value) {
        if (type != null && type.equals("string"))
            value = Common.quoteString(value);
        document.setValue(iniKey, value);
    }
    
    @Override
    public String getHoverText() {
        String text = "";
        
        text += "<b>" + iniKey;
        if (StringUtils.isNotBlank(defaultValue))
            text += "<br>&nbsp;&nbsp; = default(" + defaultValue + ")";
        if (StringUtils.isNotBlank(type))
            text += "  [" + type + "]";
        text += "</b>";  //TODO unit
        text += "<br>";
        
        if (StringUtils.isNotBlank(comment))
            text += NedCommentFormatter.makeHtmlDocu(comment, false, false, null) + "<br>\n";
        
        return HoverSupport.addHTMLStyleSheet(text);
    }
    
    public SimplifiedInifileDocument getInifileDocument() {
        return document;
    }
    
    public String getIniKey() {
        return iniKey;
    }
}

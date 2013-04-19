/**
 * Copyright (c) 2012 Opensim Ltd. All rights reserved.
 */

package com.simulte.configeditor.propertysources;

import org.omnetpp.common.engine.Common;
import org.omnetpp.common.ui.HoverSupport;
import org.omnetpp.common.util.StringUtils;
import org.omnetpp.inifile.editor.model.ConfigOption;
import org.omnetpp.inifile.editor.model.ConfigRegistry;

import com.simulte.configeditor.editors.SimplifiedInifileDocument;

/**
 * A property source that represents a configuration option (such as
 * sim-time-limit or **.typename) in an inifile.
 * 
 * @author Andras
 */
public class IniConfigOptionPropertySource implements IPropertySource {
    private SimplifiedInifileDocument document;
    private String iniKey;
    private String name;
    private ConfigOption option;
    private String defaultValue;

    public IniConfigOptionPropertySource(SimplifiedInifileDocument document, String iniKey, String defaultValue) {
        this.document = document;
        this.iniKey = iniKey;
        this.name = this.iniKey.contains(".") ? StringUtils.substringAfterLast(this.iniKey, ".") : iniKey;
        this.defaultValue = defaultValue;
        option = ConfigRegistry.getOption(name);
        if (option == null)
            option = ConfigRegistry.getPerObjectOption(name);
        if (option == null)
            throw new RuntimeException("Invalid config option " + iniKey + ": " + name + " is unknown");
        if (option.isPerObject() != this.iniKey.contains("."))
            throw new RuntimeException("Only per-object config option keys may contain a dot, and they must contain one.");
    }

    @Override
    public void initialize() {
    }

    @Override
    public String getValueForEditing() {
        String value = document.getValue(iniKey);
        if (StringUtils.isBlank(value))
            value = defaultValue;
        if (option == ConfigRegistry.CFGID_TYPENAME) {
            try {
                value = Common.parseQuotedString(value);
            }
            catch (Exception e) {
                // misquoted -- leave as it is
            }
        }
        return value;
    }

    @Override
    public void setValue(String value) {
        if (option == ConfigRegistry.CFGID_TYPENAME)
            value = Common.quoteString(value);

        if (StringUtils.isNotBlank(value) || StringUtils.isNotBlank(defaultValue))
            document.setValue(iniKey, value);
        else
            document.removeValue(iniKey);
    }

    @Override
    public String getHoverText() {
        String text = "";

        text += "<b>" + iniKey;
        if (StringUtils.isNotBlank(defaultValue))
            text += "<br>&nbsp;&nbsp; = default(" + defaultValue + ")";
        text += "</b>";
        text += "<br>";

        return HoverSupport.addHTMLStyleSheet(text);
    }
}

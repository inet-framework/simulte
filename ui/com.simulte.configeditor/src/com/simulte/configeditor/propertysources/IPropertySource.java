/**
 * Copyright (c) 2012 Opensim Ltd. All rights reserved.
 */

package com.simulte.configeditor.propertysources;

/**
 * Interface for property sources. A property source represents a value edited
 * by a field editor (see IFieldEditor), and provides get/set methods for use by
 * the field editor.
 * 
 * @author Andras
 */
public interface IPropertySource {
    /**
     * Needs to be called before the first use, but after NedResources has been
     * fully loaded.
     */
    public void initialize();

    /**
     * Returns a value that a field editor can display for immediate editing,
     * e.g. in a Text widget. This method is responsible for converting between
     * the internal (storage) format and the editing format; this conversion may
     * include adding/removing quotes, escaping/unescaping, etc.
     */
    public String getValueForEditing();

    /**
     * Writes back the edited value into the property source.
     */
    public void setValue(String value);

    /**
     * Should return HTML text.
     */
    public String getHoverText();

}
/**
 * Copyright (c) 2012 Opensim Ltd. All rights reserved.
 */

package com.simulte.configeditor.fieldeditors;

import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.widgets.Control;

import com.simulte.configeditor.propertysources.IPropertySource;

/**
 * Interface for field editors. Field editors appear as a widget on the form
 * (text, combo, spinner, etc), and lets the user edit the value provided by a
 * property source (IPropertySource). The edited value will be written back when
 * the widget loses focus.
 * 
 * @author Andras
 */
public interface IFieldEditor {
    /**
     * To be called when NedResources has been completely loaded
     */
    public void initialize();

    /**
     * Returns true if initialize() has been called, false otherwise.
     */
    public boolean isInitialized();

    /**
     * Returns the underlying control for layouting purposes. (This will be a
     * Composite for field editors containing more than one widget.)
     */
    public Control getControlForLayouting();

    /**
     * Returns the property source associated with this editor.
     */
    public IPropertySource getPropertySource();

    /**
     * Field editors should automatically write back the edited value, e.g. when
     * losing focus. This method can be used to force that action (e.g. force
     * the control to write back the value even though it still has focus).
     */
    public void commit();

    /**
     * Adds a modification listener to the underlying widgets.
     */
    public void addModifyListener(ModifyListener modifyListener);

    /**
     * Disposes the field editor, together with the underlying widgets
     */
    public void dispose();
}

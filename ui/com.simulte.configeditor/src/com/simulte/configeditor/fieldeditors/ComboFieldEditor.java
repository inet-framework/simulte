/**
 * Copyright (c) 2012 Opensim Ltd. All rights reserved.
 */

package com.simulte.configeditor.fieldeditors;

import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Control;
import org.omnetpp.common.ui.HoverSupport;

import com.simulte.configeditor.propertysources.IPropertySource;

/**
 * A field editor that wraps a combo widget.
 * 
 * @author Andras
 */
public class ComboFieldEditor extends AbstractFieldEditor {
    protected Combo combo;

    public ComboFieldEditor(Combo combo, IPropertySource propertySource, HoverSupport hoverSupport) {
        super(propertySource, hoverSupport);
        this.combo = combo;
        addFocusListenerTo(combo);
    }

    @Override
    public void addModifyListener(ModifyListener listener) {
        combo.addModifyListener(listener);
    }

    @Override
    public Control getControlForLayouting() {
        return combo;
    }

    @Override
    public void initialize() {
        super.initialize();
        addHoverSupport(combo);
        
        String value = propertySource.getValueForEditing();
        combo.setText(value);
    }    

    @Override
    public void commit() {
        String value = combo.getText();
        propertySource.setValue(value);
    }
}

/**
 * Copyright (c) 2012 Opensim Ltd. All rights reserved.
 */

package com.simulte.configeditor.fieldeditors;

import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Spinner;
import org.omnetpp.common.ui.HoverSupport;

import com.simulte.configeditor.SimuLTEPlugin;
import com.simulte.configeditor.propertysources.IPropertySource;

/**
 * A field editor that wraps a spinner widget.
 * 
 * @author Andras
 */
public class SpinnerFieldEditor extends AbstractFieldEditor {
    protected Spinner spinner;

    /**
     * Note: max spinner value is 100 by default!
     */
    public SpinnerFieldEditor(Spinner spinner, IPropertySource propertySource, HoverSupport hoverSupport) {
        super(propertySource, hoverSupport);
        this.spinner = spinner;
        addFocusListenerTo(spinner);
    }

    @Override
    public void addModifyListener(ModifyListener listener) {
        spinner.addModifyListener(listener);
    }

    @Override
    public Control getControlForLayouting() {
        return spinner;
    }
    
    @Override
    public void initialize() {
        super.initialize();
        addHoverSupport(spinner);
        
        try {
            String value = propertySource.getValueForEditing();
            int i = Integer.valueOf(value);
            spinner.setSelection(i);
            if (spinner.getSelection() != i)
                throw new RuntimeException("Could not store value in spinner -- spinner range is not enough?");
        } 
        catch (Exception e) {
            SimuLTEPlugin.logError(e);
        }
    }
    

    @Override
    public void commit() {
        int i = spinner.getSelection();
        propertySource.setValue(String.valueOf(i));
    }
}

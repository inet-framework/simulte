/**
 * Copyright (c) 2012 Opensim Ltd. All rights reserved.
 */

package com.simulte.configeditor.fieldeditors;

import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Text;
import org.omnetpp.common.ui.HoverSupport;

import com.simulte.configeditor.propertysources.IPropertySource;

/**
 * A field editor that wraps a textedit widget.
 * 
 * @author Andras
 */
public class TextFieldEditor extends AbstractFieldEditor {
    protected Text text;

    public TextFieldEditor(Text text, IPropertySource propertySource, HoverSupport hoverSupport) {
        super(propertySource, hoverSupport);
        this.text = text;
        addFocusListenerTo(text);
    }

    @Override
    public void addModifyListener(ModifyListener listener) {
        text.addModifyListener(listener);
    }

    @Override
    public Control getControlForLayouting() {
        return text;
    }

    @Override
    public void initialize() {
        super.initialize();
        addHoverSupport(text);

        String value = propertySource.getValueForEditing();
        text.setText(value);
        text.selectAll();
    }    

    @Override
    public void commit() {
        String value = text.getText();
        propertySource.setValue(value);
    }
}

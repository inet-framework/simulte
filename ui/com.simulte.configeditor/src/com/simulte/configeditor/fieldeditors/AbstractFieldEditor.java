/**
 * Copyright (c) 2012 Opensim Ltd. All rights reserved.
 */

package com.simulte.configeditor.fieldeditors;

import org.eclipse.swt.events.FocusEvent;
import org.eclipse.swt.events.FocusListener;
import org.eclipse.swt.widgets.Control;
import org.omnetpp.common.ui.HoverInfo;
import org.omnetpp.common.ui.HoverSupport;
import org.omnetpp.common.ui.HtmlHoverInfo;
import org.omnetpp.common.ui.IHoverInfoProvider;
import com.simulte.configeditor.propertysources.IPropertySource;

/**
 * Abstract base class for field editors. See IFieldEditor for more info.
 *
 * @author Andras
 */
public abstract class AbstractFieldEditor implements IFieldEditor {
    protected boolean isInitialized = false;
    protected IPropertySource propertySource;
    protected HoverSupport hoverSupport;

    public AbstractFieldEditor(IPropertySource propertySource, HoverSupport hoverSupport) {
        this.propertySource = propertySource;
        this.hoverSupport = hoverSupport;
    }

    /**
     * Fill the control.
     *
     * Note: may only be called when NedResources has been fully loaded! (see isLoadingInProgress())
     */
    public void initialize() {
        propertySource.initialize();
        isInitialized = true;
    }

    @Override
    public boolean isInitialized() {
        return isInitialized;
    }

    @Override
    public IPropertySource getPropertySource() {
        return propertySource;
    }

    @Override
    public void dispose() {
        // default implementation which is usually OK, but maybe not if some compound control is used
        getControlForLayouting().dispose();
        hoverSupport.forget(getControlForLayouting());
    }

    /**
     * Utility function for subclasses
     */
    protected void addFocusListenerTo(Control control) {
        control.addFocusListener(new FocusListener() {
            public void focusGained(FocusEvent e) {
            }

            public void focusLost(FocusEvent e) {
                commit();
            }
        });
    }

    /**
     * Utility function for subclasses
     */
    protected void addHoverSupport(Control control) {
        hoverSupport.adapt(control, new IHoverInfoProvider() {

            @Override
            public HoverInfo getHoverFor(Control control, int x, int y) {
                return new HtmlHoverInfo(propertySource.getHoverText());
            }
        });
    }
}

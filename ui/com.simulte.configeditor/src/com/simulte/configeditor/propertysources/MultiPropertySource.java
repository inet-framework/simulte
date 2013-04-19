/**
 * Copyright (c) 2012 Opensim Ltd. All rights reserved.
 */

package com.simulte.configeditor.propertysources;

import org.eclipse.core.runtime.Assert;

/**
 * A property source that wraps multiple property sources. Setting the property
 * will write the value into all sub-propertysources; getting will return the
 * value from the first one.
 * 
 * Can be used if the parameterization of a model is redundant, i.e. the same
 * value must be assigned to multiple parameters.
 * 
 * @author Andras
 */
public class MultiPropertySource implements IPropertySource {
    private IPropertySource[] propertySources;

    public MultiPropertySource(IPropertySource[] propertySources) {
        Assert.isTrue(propertySources != null && propertySources.length > 0);
        this.propertySources = propertySources;
    }

    public void initialize() {
        for (IPropertySource p : propertySources)
            p.initialize();
    }

    public String getValueForEditing() {
        return propertySources[0].getValueForEditing();
    }

    public void setValue(String value) {
        for (IPropertySource p : propertySources)
            p.setValue(value);
    }

    public String getHoverText() {
        return propertySources[0].getHoverText();
    }

}
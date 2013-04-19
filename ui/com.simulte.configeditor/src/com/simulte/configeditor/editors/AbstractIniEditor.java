/**
 * Copyright (c) 2012 Opensim Ltd. All rights reserved.
 */

package com.simulte.configeditor.editors;

import java.io.ByteArrayInputStream;
import java.io.IOException;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.resources.IProject;
import org.eclipse.core.resources.IResource;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.ui.IEditorInput;
import org.eclipse.ui.IEditorSite;
import org.eclipse.ui.IFileEditorInput;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.part.EditorPart;
import org.omnetpp.inifile.editor.model.ParseException;

import com.simulte.configeditor.SimuLTEPlugin;

/**
 * Base class for form-based inifile editors.
 *  
 * @author Andras
 */
public abstract class AbstractIniEditor extends EditorPart {
    protected SimplifiedInifileDocument document = null;
    protected boolean dirty = false;

    public AbstractIniEditor() {
		super();
	}
	
    @Override
    public void init(IEditorSite site, IEditorInput input) throws PartInitException {
        if (!(input instanceof IFileEditorInput))
            throw new PartInitException(new Status(IStatus.ERROR, SimuLTEPlugin.PLUGIN_ID, "Unsupported editor input: " + input.getClass().getSimpleName()));
        
        IFile file = ((IFileEditorInput) input).getFile();

        setSite(site);
        setInput(input);
        setPartName(file.getName());
    }

    protected SimplifiedInifileDocument parseIni(IFile file) throws CoreException, ParseException, IOException {
        IProject project = file.getProject();
        if (!project.getWorkspace().isTreeLocked())
            file.refreshLocal(IResource.DEPTH_ZERO, null);

        SimplifiedInifileDocument doc = new SimplifiedInifileDocument(file);
        doc.parse();
        return doc;
    }
    
    public void setDirty(boolean dirty) {
        this.dirty = dirty;
        firePropertyChange(PROP_DIRTY);
    }
    
	public void dispose() {
		super.dispose();
	}

    @Override
    public boolean isDirty() {
        return dirty;
    }

    @Override
    public boolean isSaveAsAllowed() {
        return false;
    }

    @Override
    public void doSave(IProgressMonitor monitor) {
        try {
            String content = document.getContents();

            // save it
            IFile file = ((IFileEditorInput) getEditorInput()).getFile();
            if (!file.exists())
                file.create(new ByteArrayInputStream(content.getBytes()), IFile.FORCE, null);
            else
                file.setContents(new ByteArrayInputStream(content.getBytes()), IFile.FORCE, null);
            setDirty(false);
        }
        catch (Exception e) {  // catches: ParserConfigurationException,ClassCastException,ClassNotFoundException,InstantiationException,IllegalAccessException
            throw new RuntimeException("Cannot save file: " + e.getMessage(), e);
        }
    }

    @Override
    public void doSaveAs() {
        throw new UnsupportedOperationException(); // for now
    }

}

package com.simulte.configeditor.wizards;

import java.io.ByteArrayInputStream;
import java.io.InputStream;
import org.eclipse.core.resources.IFile;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.IWorkbench;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchWindow;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.dialogs.WizardNewFileCreationPage;
import org.eclipse.ui.ide.IDE;
import com.simulte.configeditor.SimuLTEPlugin;
import com.simulte.configeditor.editors.SimuLTEConfigEditor;

/**
 * Create a blank ini file for the SimuLTE config editor.
 */
public class NewLteConfigWizardPage1 extends WizardNewFileCreationPage {
    private IWorkbench workbench;

    public NewLteConfigWizardPage1(IWorkbench aWorkbench, IStructuredSelection selection) {
        super("page1", selection);
        setTitle("Create SimuLTE Configuration");
        setDescription("This wizard creates a blank SimuLTE configuration file.");
        setImageDescriptor(SimuLTEPlugin.getImageDescriptor("icons/full/wizban/newinifile.png"));
        workbench = aWorkbench;

        setFileExtension("ini");
        setFileName("omnetpp.ini"); // append "1", "2" etc if not unique in that folder
        // XXX set initial folder to either selection, or folder of input of current editor 
        // setContainerFullPath(path)
    }

    @Override
    public void createControl(Composite parent) {
        super.createControl(parent);
        setPageComplete(validatePage());
    }

    @Override
    protected InputStream getInitialContents() {
        String contents = "\n";
        return new ByteArrayInputStream(contents.getBytes());
    }

    public boolean finish() {
        IFile newFile = createNewFile();
        if (newFile == null)
            return false; // creation was unsuccessful

        // Since the file resource was created fine, open it for editing
        try {
            IWorkbenchWindow dwindow = workbench.getActiveWorkbenchWindow();
            IWorkbenchPage page = dwindow.getActivePage();
            if (page != null)
                IDE.openEditor(page, newFile, SimuLTEConfigEditor.ID);
        }
        catch (PartInitException e) {
            SimuLTEPlugin.logError(e);
            return false;
        }
        return true;
    }

}

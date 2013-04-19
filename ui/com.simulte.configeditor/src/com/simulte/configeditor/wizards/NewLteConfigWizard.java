package com.simulte.configeditor.wizards;

import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.wizard.Wizard;
import org.eclipse.ui.INewWizard;
import org.eclipse.ui.IWorkbench;

/**
 * Wizard for SimuLTE config files. The wizard creates one file with the extension "ini"
 * and launches the editor.
 */
public class NewLteConfigWizard extends Wizard implements INewWizard {
    private NewLteConfigWizardPage1 page1 = null;
    private IStructuredSelection selection;
    private IWorkbench workbench;

    @Override
    public void addPages() {
        page1 = new NewLteConfigWizardPage1(workbench, selection);
        addPage(page1);
    }

    public void init(IWorkbench aWorkbench, IStructuredSelection currentSelection) {
        workbench = aWorkbench;
        selection = currentSelection;
        setWindowTitle("New Ini File");
    }

    @Override
    public boolean performFinish() {
        return page1.finish();
    }
}

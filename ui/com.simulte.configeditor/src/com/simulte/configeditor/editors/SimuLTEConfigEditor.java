/**
 * Copyright (c) 2012 Opensim Ltd. All rights reserved.
 */

package com.simulte.configeditor.editors;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import org.apache.commons.lang.ObjectUtils;
import org.eclipse.core.resources.IFile;
import org.eclipse.core.runtime.Assert;
import org.eclipse.core.runtime.CoreException;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.IEditorInput;
import org.eclipse.ui.IEditorSite;
import org.eclipse.ui.IFileEditorInput;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.ScrolledForm;
import org.eclipse.ui.forms.widgets.Section;
import org.omnetpp.common.ui.HoverSupport;
import org.omnetpp.inifile.editor.model.ParseException;
import org.omnetpp.ned.core.INedResources;
import org.omnetpp.ned.core.NedResourcesPlugin;
import org.w3c.dom.Attr;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;

import com.simulte.configeditor.SimuLTEPlugin;
import com.simulte.configeditor.fieldeditors.ComboFieldEditor;
import com.simulte.configeditor.fieldeditors.IFieldEditor;
import com.simulte.configeditor.fieldeditors.SpinnerFieldEditor;
import com.simulte.configeditor.fieldeditors.TextFieldEditor;
import com.simulte.configeditor.propertysources.IPropertySource;
import com.simulte.configeditor.propertysources.IXmlAttributeProvider;
import com.simulte.configeditor.propertysources.IniConfigOptionPropertySource;
import com.simulte.configeditor.propertysources.IniParameterPropertySource;
import com.simulte.configeditor.propertysources.MultiPropertySource;
import com.simulte.configeditor.propertysources.ParsedXmlIniParameter;
import com.simulte.configeditor.propertysources.XmlAttributePropertySource;

/**
 * SimuLTE config editor. If you need to change something, chances are that
 * only populateControls() needs to be edited.
 * 
 * @author Andras
 */
//TODO: SHOULD ONLY OPEN INIFILES THAT IT RECOGNIZES! (add some guard to the saved file)
//TODO implement "reset to default" (or at least: show default in tooltips)
//TODO move all generic parts into the base class
public class SimuLTEConfigEditor extends AbstractIniEditor {
    public static final String ID = "org.omnetpp.simulte.editors.SimuLTEConfigEditor";
    
    protected ScrolledForm form;
    protected FormToolkit toolkit = new FormToolkit(Display.getDefault());
    protected HoverSupport hoverSupport = new HoverSupport();

    protected List<IFieldEditor> fieldEditors = new ArrayList<IFieldEditor>();
    protected List<ParsedXmlIniParameter> xmlParameters = new ArrayList<ParsedXmlIniParameter>();

    protected List<IFieldEditor> ueMobilityParamsFieldEditors = new ArrayList<IFieldEditor>();
    protected List<Control> ueMobilityParamsExtraControls = new ArrayList<Control>();

    protected List<IFieldEditor> ueAppParamsFieldEditors = new ArrayList<IFieldEditor>();
    protected List<Control> ueAppParamsExtraControls = new ArrayList<Control>();
    
    protected List<IFieldEditor> serverAppParamsFieldEditors = new ArrayList<IFieldEditor>();
    protected List<Control> serverAppParamsExtraControls = new ArrayList<Control>();

    private boolean populateControlsDone = false;
    private static boolean eventFilterAdded = false;  

    protected ModifyListener modifyListener = new ModifyListener() {
        @Override
        public void modifyText(ModifyEvent e) {
            setDirty(true);
        }
    };

    static final String NED_DEPLOYER = "lte.corenetwork.deployer.LteDeployer"; 
    static final String NED_STATIONARY_MOBILITY = "inet.mobility.models.StationaryMobility"; 
    static final String NED_LINEAR_MOBILITY = "inet.mobility.models.LinearMobility"; 
    static final String NED_RANDOMWP_MOBILITY = "inet.mobility.models.RandomWPMobility"; 
    static final String NED_UDP_BASIC_APP = "inet.applications.udpapp.UDPBasicApp"; 
    static final String NED_UDP_ECHO_APP = "inet.applications.udpapp.UDPEchoApp"; 
    static final String NED_UDP_VIDEO_CLI_APP = "inet.applications.udpapp.UDPVideoStreamCli"; 
    static final String NED_UDP_VIDEO_SVR_APP = "inet.applications.udpapp.UDPVideoStreamSvr"; 
    static final String NED_LTE_MAC_BASE = "lte.stack.mac.LteMacBase";
    static final String NED_LTE_MAC_ENB = "lte.stack.mac.LteMacEnb";
    static final String NED_LTE_FEEDBACKGENERATOR = "lte.stack.phy.feedback.LteDlFeedbackGenerator";
    static final String NED_LTE_UE = "lte.corenetwork.nodes.Ue";
    static final String NED_LTE_NETWORK = "lte.networks.LteNetwork";
    static final String NED_LTE_APPS_GAMING = "lte.apps.gaming.Gaming";
    static final String NED_LTE_APPS_VODUDPCLIENT = "lte.apps.vod.VoDUDPClient";
    static final String NED_LTE_APPS_VODUDPSERVER = "lte.apps.vod.VoDUDPServer";
    static final String NED_LTE_APPS_VOIP_VOIPSENDER = "lte.apps.voip.VoIPSender";
    static final String NED_LTE_APPS_VOIP_VOIPRECEIVER = "lte.apps.voip.VoIPReceiver";

    // see config_pisa.xml
    protected static class AnalogueModelParameterAttrProvider implements IXmlAttributeProvider {
        private String paramName;
        private String paramType;

        public AnalogueModelParameterAttrProvider(String paramName, String paramType) {
            this.paramName = paramName;
            this.paramType = paramType;
        }

        @Override
        public Attr getEditedAttr(Document xmlDoc) {
            Element analogueModelsElement = getOrCreateFirstChildByTagName(xmlDoc.getDocumentElement(), "AnalogueModels");
            Element analogueModelElement = getOrCreateFirstChildByTagName(analogueModelsElement, "AnalogueModel", "type", "itu");
            Element parameterElement = getOrCreateFirstChildByTagName(analogueModelElement, "parameter", "name", paramName);
            parameterElement.setAttribute("type", paramType);
            return getOrCreateAttr(parameterElement, "value");
        }

        @Override
        public String getAttrPathInfo() {
            return "<AnalogueModels> / <AnalogueModel type='itu'> / <parameter name='" + paramName + "' type='" + paramType + "' value='...'>";
        }
    }

    protected static class ChannelModelParameterAttrProvider implements IXmlAttributeProvider {
        private String paramName;
        private String paramType;

        public ChannelModelParameterAttrProvider(String paramName, String paramType) {
            this.paramName = paramName;
            this.paramType = paramType;
        }

        @Override
        public Attr getEditedAttr(Document xmlDoc) {
            Element channelModelElement = getOrCreateFirstChildByTagName(xmlDoc.getDocumentElement(), "ChannelModel", "type", "REAL");
            Element parameterElement = getOrCreateFirstChildByTagName(channelModelElement, "parameter", "name", paramName);
            parameterElement.setAttribute("type", paramType);
            return getOrCreateAttr(parameterElement, "value");
        }

        @Override
        public String getAttrPathInfo() {
            return "<ChannelModel type='REAL'> / <parameter name='" + paramName + "' type='" + paramType + "' value='...'>";
        }
    }

    protected static class DeciderParameterAttrProvider implements IXmlAttributeProvider {
        private String paramName;
        private String paramType;

        public DeciderParameterAttrProvider(String paramName, String paramType) {
            this.paramName = paramName;
            this.paramType = paramType;
        }

        @Override
        public Attr getEditedAttr(Document xmlDoc) {
            Element channelModelElement = getOrCreateFirstChildByTagName(xmlDoc.getDocumentElement(), "LteDecider", "type", "LtePisaDecider");
            Element parameterElement = getOrCreateFirstChildByTagName(channelModelElement, "parameter", "name", paramName);
            parameterElement.setAttribute("type", paramType);
            return getOrCreateAttr(parameterElement, "value");
        }

        @Override
        public String getAttrPathInfo() {
            return "<LteDecider type='LtePisaDecider'> / <parameter name='" + paramName + "' type='" + paramType + "' value='...'>";
        }
    }
    
    protected static class FeedbackComputationParameterAttrProvider implements IXmlAttributeProvider {
        private String paramName;
        private String paramType;

        public FeedbackComputationParameterAttrProvider(String paramName, String paramType) {
            this.paramName = paramName;
            this.paramType = paramType;
        }

        @Override
        public Attr getEditedAttr(Document xmlDoc) {
            Element feedbackComputationElement = getOrCreateFirstChildByTagName(xmlDoc.getDocumentElement(), "FeedbackComputation", "type", "REAL");
            Element parameterElement = getOrCreateFirstChildByTagName(feedbackComputationElement, "parameter", "name", paramName);
            parameterElement.setAttribute("type", paramType);
            return getOrCreateAttr(parameterElement, "value");
        }

        @Override
        public String getAttrPathInfo() {
            return "<FeedbackComputation type='REAL'> / <parameter name='" + paramName + "' type='" + paramType + "' value='...'>";
        }
    }

    
    @Override
    public void init(IEditorSite site, IEditorInput input) throws PartInitException {
        super.init(site, input);
    }
    
    @Override
    public void createPartControl(Composite parent) {

        installEventFilter();
        
        form = toolkit.createScrolledForm(parent);
        form.setText("SimuLTE Level-1 Configuration Editor");
        
        IFile file = ((IFileEditorInput) getEditorInput()).getFile();
        try {
            document = parseIni(file);
        }
        catch (CoreException e) {
            displayMessageLater(file, e);
            SimuLTEPlugin.logError(e);
        }
        catch (ParseException e) {
            displayMessageLater(file, e);
            SimuLTEPlugin.logError(e);
        }
        catch (IOException e) {
            displayMessageLater(file, e);
            SimuLTEPlugin.logError(e);
        }

        // if file couldn't be loaded, use blank dummy file to ensure the rest of the editor doesn't crash with NPE
        if (document == null)
            document = new SimplifiedInifileDocument();  // dummy document
            
        // add commonly needed entries into the file
        document.setValue("**.ue*.masterId", "1"); 
        document.setValue("**.ue*.macCellId", "1");
        document.setValue("*.ue*.mobility.initFromDisplayString", "false");
        document.setValue("*.ue*.mobility.initialZ", "0");
        document.setValue("*.eNodeB.mobility.initialX", "0");  // workaround for Omnet bug: NaN parameters are handled incorrectly
        document.setValue("*.eNodeB.mobility.initialY", "0");  // position is taken from the display string
        document.setValue("*.eNodeB.mobility.initialZ", "0");
        document.setValue("**.numUdpApps", "1");

        // add fields (must be after document loading, as they need the document!)
        buildForm(form.getBody());
        form.reflow(true);

        populateControls();
    }


    /**
     * Prevents the mouse wheel from changing the value of Combo and Spinner fields on Linux
     */
    private void installEventFilter() {
        if (!eventFilterAdded) {
            eventFilterAdded = true;
            Display.getDefault().addFilter(SWT.MouseWheel, new Listener() {
                @Override
                public void handleEvent(Event event) {
                    if (event.widget instanceof Combo || event.widget instanceof Spinner)
                        event.doit = false;
                }
            });
        }
    }

    protected void displayMessageLater(final IFile file, final Exception e) {
        Display.getCurrent().asyncExec(new Runnable() {
            @Override
            public void run() {
                MessageDialog.openError(getSite().getShell(), "Error", "Error opening " + file.getFullPath() + ": " + e.getMessage());
            }
        });
    }

    protected void buildForm(Composite composite) {
        composite.setLayout(new GridLayout(1, false));
        createWrappingLabel(composite, 
                "This form-based editor lets you configure a single-cell simulated LTE network. " +
                "Form fields are provided to configure one eNodeB, several UEs and one server on the backhaul; " +
                "selected applications (one application per UE and one on the server); and selected " +
                "mobility patterns (all UEs move according to a common mobility pattern)." +
                "\n\n" +
                "NOTE: The above restrictions are only limitations of this editor: when editing the configuration files manually " +
                "(in a text editor or in the OMNEST Inifile Editor), it is possible to have several cells, arbitrary routers, " +
                "servers and other nodes to model the internet; arbitrary applications; arbitrary and diverse mobility patterns; " +
                "and also, more LTE parameters are available.", 1);
        createLabel(composite, "", 1); // spacer

        int approximateLineHeight = measureDefaultTextControlHeight(composite) + new GridLayout().verticalSpacing;
        
        ParsedXmlIniParameter channelModelXml = createParsedXmlIniParameter("**.channelModel", "lte.stack.phy.LtePhy");
        ParsedXmlIniParameter deciderXml = createParsedXmlIniParameter("**.decider", "lte.stack.phy.LtePhy");
        ParsedXmlIniParameter analogModelsXml = createParsedXmlIniParameter("**.analogueModels", "lte.stack.phy.LtePhy");
        ParsedXmlIniParameter feedbackXml = createParsedXmlIniParameter("**.feedbackComputation", "lte.stack.phy.LtePhyEnb");

        Composite sectionBody = (Composite)createSection(composite, "Simulation Control", 1).getClient();
        Composite c = createComposite(sectionBody, 1, 2);
        createLabel(c, "Network:");
        createTextIniOptionEditor(c, "network", "lte.networks.LteNetwork");
        createWrappingLabel(c, "NED type name of the LTE network model, as created in the NED graphical editor. UEs, eNodeB and the server must be called \"ue*\", \"eNodeB\" and \"server\", respectively.", 2);
        createLabel(c, "Simulation time limit (s):");
        createTextIniOptionEditor(c, "sim-time-limit", "");
        createLabel(c, "CPU time limit (s):");
        createTextIniOptionEditor(c, "cpu-time-limit", "");

        sectionBody = (Composite)createSection(composite, "Physical Layer", 3).getClient();

        Group group = createGroup(sectionBody, "Channel model:", 2);
        createLabel(group, "Carrier Frequency (GHz):");
        createTextXmlAttrEditor(group,  channelModelXml, new ChannelModelParameterAttrProvider("carrierFrequency", "double"), "2.1"); //XXX same as channelControl's?
        createLabel(group, "Scenario:");
        createComboXmlAttrEditor(group, new IPropertySource[] {
                new XmlAttributePropertySource(channelModelXml, new ChannelModelParameterAttrProvider("scenario", "string"), "URBAN_MACROCELL"),
                new XmlAttributePropertySource(analogModelsXml, new AnalogueModelParameterAttrProvider("scenario", "string"), "URBAN_MACROCELL"),
        }, "INDOOR_HOTSPOT,URBAN_MICROCELL,URBAN_MACROCELL,RURAL_MACROCELL,SUBURBAN_MACROCELL");
        createLabel(group, "eNodeB height (m):");
        createTextXmlAttrEditor(group, new IPropertySource[] {
                new XmlAttributePropertySource(channelModelXml, new ChannelModelParameterAttrProvider("nodeb-height", "double"), "25"),
                new XmlAttributePropertySource(analogModelsXml, new AnalogueModelParameterAttrProvider("nodeb-height", "double"), "25"),
        });
        createLabel(group, "Building height (m):");
        createTextXmlAttrEditor(group, new IPropertySource[] {
                new XmlAttributePropertySource(channelModelXml, new ChannelModelParameterAttrProvider("building-height", "double"), "20"),
                new XmlAttributePropertySource(analogModelsXml, new AnalogueModelParameterAttrProvider("building-height", "double"), "20"),
        });
        createLabel(group, "Shadowing:");
        createBoolXmlAttrEditor(group, new IPropertySource[] {
                new XmlAttributePropertySource(channelModelXml, new ChannelModelParameterAttrProvider("shadowing", "bool"), "true"),
                new XmlAttributePropertySource(analogModelsXml, new AnalogueModelParameterAttrProvider("shadowing", "bool"), "true"),
        });
        createLabel(group, "Automatically switch LOS/NLOS");
        createBoolXmlAttrEditor(group, channelModelXml, new ChannelModelParameterAttrProvider("dynamic-los", "bool"), false);
        createLabel(group, "Force LOS");
        createBoolXmlAttrEditor(group, channelModelXml, new ChannelModelParameterAttrProvider("fixed-los", "bool"), false);  //XXX
        createLabel(group, "Fading");
        createBoolXmlAttrEditor(group, channelModelXml, new ChannelModelParameterAttrProvider("fading", "bool"), true);
        createLabel(group, "Fading type");
        createComboXmlAttrEditor(group, channelModelXml, new ChannelModelParameterAttrProvider("fading-type", "string"), "JAKES,RAYGHLEY", "JAKES");
        createLabel(group, "Number of paths (for JAKES fading)");
        createTextXmlAttrEditor(group,  channelModelXml, new ChannelModelParameterAttrProvider("fading-paths", "long"), "6");
        createLabel(group, "Inter-cell interference");
        createBoolXmlAttrEditor(group, channelModelXml, new ChannelModelParameterAttrProvider("extCell-interference", "double"), false);
        createLabel(group, "In-cell interference");
        createBoolXmlAttrEditor(group, channelModelXml, new ChannelModelParameterAttrProvider("inCell-interference", "double"), false);
        
        group = createGroup(sectionBody, "Equipment Characteristics:", 2);
        createLabel(group, "UE antenna gain (dBi):");
        createTextXmlAttrEditor(group, new IPropertySource[] {
                new XmlAttributePropertySource(channelModelXml, new ChannelModelParameterAttrProvider("antennaGainUe", "double"), "0"),
                new XmlAttributePropertySource(deciderXml, new DeciderParameterAttrProvider("antennaGainUe", "double"), "0")
        });
        createLabel(group, "eNodeB antenna gain (dBi):");
        createTextXmlAttrEditor(group, new IPropertySource[] {
                new XmlAttributePropertySource(channelModelXml, new ChannelModelParameterAttrProvider("antennGainEnB", "double"), "18"),
                new XmlAttributePropertySource(deciderXml, new DeciderParameterAttrProvider("antennaGainEnB", "double"), "18")
        });
        createLabel(group, "Antenna Gain of Micro node (dBi)");
        createTextXmlAttrEditor(group,  channelModelXml, new ChannelModelParameterAttrProvider("antennGainMicro", "double"), "5");
        createLabel(group, "UE noise figure (dB)");
        createTextXmlAttrEditor(group, new IPropertySource[] {
                new XmlAttributePropertySource(channelModelXml, new ChannelModelParameterAttrProvider("ue-noise-figure", "double"), "7"),
                new XmlAttributePropertySource(deciderXml, new DeciderParameterAttrProvider("ue-noise-figure", "double"), "7")
        });
        createLabel(group, "eNodeB noise figure (dB)");
        createTextXmlAttrEditor(group, new IPropertySource[] {
                new XmlAttributePropertySource(channelModelXml, new ChannelModelParameterAttrProvider("bs-noise-figure", "double"), "5"),
                new XmlAttributePropertySource(deciderXml, new DeciderParameterAttrProvider("bs-noise-figure", "double"), "5") 
        });
        createLabel(group, "Thermal Noise for\n10 MHz of Bandwidth (dB)");
        createTextXmlAttrEditor(group, new IPropertySource[] {
                new XmlAttributePropertySource(channelModelXml, new ChannelModelParameterAttrProvider("thermalNoise", "double"), "-104.5"),
                new XmlAttributePropertySource(deciderXml, new DeciderParameterAttrProvider("thermalNoise", "double"), "-104.5")
        });
        createLabel(group, "Cable Loss (dB)");
        createTextXmlAttrEditor(group, new IPropertySource[] {
                new XmlAttributePropertySource(channelModelXml, new ChannelModelParameterAttrProvider("cable-loss", "double"), "2"),
                new XmlAttributePropertySource(deciderXml, new DeciderParameterAttrProvider("cable-loss", "double"), "2")
        });
        createLabel(group, "MIMO Enablement:", 2);
        createLabel(group, "Lambda Min Threshold");
        createTextXmlAttrEditor(group, new IPropertySource[] {
                new XmlAttributePropertySource(channelModelXml, new ChannelModelParameterAttrProvider("lambdaMinTh", "double"), "0.02"),
                new XmlAttributePropertySource(deciderXml, new DeciderParameterAttrProvider("lambdaMinTh", "double"), "0.02"),
                new XmlAttributePropertySource(feedbackXml, new FeedbackComputationParameterAttrProvider("lambdaMinTh", "double"), "0.02")
        });
        createLabel(group, "Lambda Max Threshold");
        createTextXmlAttrEditor(group, new IPropertySource[] {
                new XmlAttributePropertySource(channelModelXml, new ChannelModelParameterAttrProvider("lambdaMaxTh", "double"), "0.2"),
                new XmlAttributePropertySource(deciderXml, new DeciderParameterAttrProvider("lambdaMaxTh", "double"), "0.2"),
                new XmlAttributePropertySource(feedbackXml, new FeedbackComputationParameterAttrProvider("lambdaMaxTh", "double"), "0.2")
        });
        createLabel(group, "Lambda Ratio");
        createTextXmlAttrEditor(group, new IPropertySource[] {
                new XmlAttributePropertySource(channelModelXml, new ChannelModelParameterAttrProvider("lambdaRatioTh", "double"), "20"),
                new XmlAttributePropertySource(deciderXml, new DeciderParameterAttrProvider("lambdaRatioTh", "double"), "20"),
                new XmlAttributePropertySource(feedbackXml, new FeedbackComputationParameterAttrProvider("lambdaRatioTh", "double"), "20")
        });
        
        sectionBody = (Composite)createSection(composite, "MAC Layer (eNodeB and UE)", 3).getClient();
        group = createGroup(sectionBody, "HARQ", 2);
        createLabel(group, "Num HARQ processes:");
        createTextIniFieldEditor(group, "*.*.nic.mac.harqProcesses", NED_LTE_MAC_BASE, null);
        createLabel(group, "Max HARQ Retransmissions:");
        createTextIniFieldEditor(group, "*.*.nic.mac.maxHarqRtx", NED_LTE_MAC_BASE, null);
        createLabel(group, "HARQ reduction:");
        createTextXmlAttrEditor(group, new IPropertySource[] {
                new XmlAttributePropertySource(channelModelXml, new ChannelModelParameterAttrProvider("harqReduction", "double"), "0.2"), // or 0.5
                new XmlAttributePropertySource(deciderXml, new DeciderParameterAttrProvider("harqReduction", "double"), "0.2") // or 0.5
        });

        group = createGroup(sectionBody, "Misc", 2);
        createLabel(group, "Queue size:");
        createTextIniFieldEditor(group, "*.*.nic.mac.queueSize", NED_LTE_MAC_BASE, null);
        createLabel(group, "Max bytes/TTI:");
        createTextIniFieldEditor(group, "*.*.nic.mac.maxBytesPerTti", NED_LTE_MAC_BASE, null);
        createLabel(group, "MU-Mimo:");
        createBoolIniFieldEditor(group, "*.*.nic.mac.muMimo", NED_LTE_MAC_BASE, null);
        createLabel(group, "Target block error rate:");
        createTextXmlAttrEditor(group, new IPropertySource[] {
                new XmlAttributePropertySource(channelModelXml, new ChannelModelParameterAttrProvider("targetBler", "double"), "0.001"),
                new XmlAttributePropertySource(deciderXml, new DeciderParameterAttrProvider("targetBler", "double"), "0.001"),
                new XmlAttributePropertySource(feedbackXml, new FeedbackComputationParameterAttrProvider("targetBler", "double"), "0.001")
        });
        
        sectionBody = (Composite)createSection(composite, "eNodeB", 3).getClient();
        
        Composite column = createComposite(sectionBody, 1, 1);
        group = createGroup(column, "Downlink:", 2);
        createLabel(group, "Number of resource blocks (carriers):");
        createSpinnerIniFieldEditor(group, "*.eNodeB.deployer.numRbDl", NED_DEPLOYER, null);
        createLabel(group, "Subcarriers per resource block:");
        createSpinnerIniFieldEditor(group, "*.eNodeB.deployer.rbyDl", NED_DEPLOYER, null);
        createLabel(group, "Symbols per slot:");
        createSpinnerIniFieldEditor(group, "*.eNodeB.deployer.rbxDl", NED_DEPLOYER, null);
        createLabel(group, "Pilot symbols per slot:");
        createSpinnerIniFieldEditor(group, "*.eNodeB.deployer.rbPilotDl", NED_DEPLOYER, null);
        createLabel(group, "Signalling symbols per slot:");
        createSpinnerIniFieldEditor(group, "*.eNodeB.deployer.signalDl", NED_DEPLOYER, null);
        createLabel(group, "Scheduling discipline:");
        createComboIniFieldEditor(group, "*.eNodeB.nic.mac.schedulingDisciplineDl", NED_LTE_MAC_ENB, "MAXCI,DRR,PF", null);
        createLabel(group, "Grant type / Conversational:");
        createComboIniFieldEditor(group, "*.eNodeB.nic.mac.grantTypeConversationalDl", NED_LTE_MAC_ENB, "FITALL,URGENT,FIXED", null);
        createLabel(group, "Grant size / Conversational:");
        createTextIniFieldEditor(group, "*.eNodeB.nic.mac.grantSizeConversationalDl", NED_LTE_MAC_ENB, null);
        createLabel(group, "Grant type / Streaming:");
        createComboIniFieldEditor(group, "*.eNodeB.nic.mac.grantTypeStreamingDl", NED_LTE_MAC_ENB, "FITALL,URGENT,FIXED", null);
        createLabel(group, "Grant size / Streaming:");
        createTextIniFieldEditor(group, "*.eNodeB.nic.mac.grantSizeStreamingDl", NED_LTE_MAC_ENB, null);
        createLabel(group, "Grant type / Interactive:");
        createComboIniFieldEditor(group, "*.eNodeB.nic.mac.grantTypeInteractiveDl", NED_LTE_MAC_ENB, "FITALL,URGENT,FIXED", null);
        createLabel(group, "Grant size / Interactive:");
        createTextIniFieldEditor(group, "*.eNodeB.nic.mac.grantSizeInteractiveDl", NED_LTE_MAC_ENB, null);
        createLabel(group, "Grant type / Background:");
        createComboIniFieldEditor(group, "*.eNodeB.nic.mac.grantTypeBackgroundDl", NED_LTE_MAC_ENB, "FITALL,URGENT,FIXED", null);
        createLabel(group, "Grant size / Background:");
        createTextIniFieldEditor(group, "*.eNodeB.nic.mac.grantSizeBackgroundDl", NED_LTE_MAC_ENB, null);

        group = createGroup(column, "Other:", 2);
//TODO:        createLabel(group, "Micro cell:");
//TODO:        createTextIniFieldEditor(group, "*.eNodeB.deployer.microCell", NED_DEPLOYER, toolkit)); // TODO true/false (checkbox)
        createLabel(group, "Number of logical bands:");
        createSpinnerIniFieldEditor(group, "*.eNodeB.deployer.numBands", NED_DEPLOYER, null);
        createLabel(group, "Number of preferred bands:");
        createSpinnerIniFieldEditor(group, "*.eNodeB.deployer.numPreferredBands", NED_DEPLOYER, null);

        group = createGroup(sectionBody, "Uplink:", 2);
        createLabel(group, "Number of resource blocks (carriers):");
        createSpinnerIniFieldEditor(group, "*.eNodeB.deployer.numRbUl", NED_DEPLOYER, null);
        createLabel(group, "Subcarriers per resource block:");
        createSpinnerIniFieldEditor(group, "*.eNodeB.deployer.rbyUl", NED_DEPLOYER, null);
        createLabel(group, "Symbols per slot:");
        createSpinnerIniFieldEditor(group, "*.eNodeB.deployer.rbxUl", NED_DEPLOYER, null);
        createLabel(group, "Pilot symbols per slot:");
        createSpinnerIniFieldEditor(group, "*.eNodeB.deployer.rbPilotUl", NED_DEPLOYER, null);
        createLabel(group, "Signalling symbols per slot:");
        createSpinnerIniFieldEditor(group, "*.eNodeB.deployer.signalUl", NED_DEPLOYER, null);

        createLabel(group, "Scheduling discipline:");
        createComboIniFieldEditor(group, "*.eNodeB.nic.mac.schedulingDisciplineUl", NED_LTE_MAC_ENB, "MAXCI,DRR,PF", null);
        createLabel(group, "Grant type / Conversational:");
        createComboIniFieldEditor(group, "*.eNodeB.nic.mac.grantTypeConversationalUl", NED_LTE_MAC_ENB, "FITALL,URGENT,FIXED", null);
        createLabel(group, "Grant size / Conversational:");
        createTextIniFieldEditor(group, "*.eNodeB.nic.mac.grantSizeConversationalUl", NED_LTE_MAC_ENB, null);
        createLabel(group, "Grant type / Streaming:");
        createComboIniFieldEditor(group, "*.eNodeB.nic.mac.grantTypeStreamingUl", NED_LTE_MAC_ENB, "FITALL,URGENT,FIXED", null);
        createLabel(group, "Grant size / Streaming:");
        createTextIniFieldEditor(group, "*.eNodeB.nic.mac.grantSizeStreamingUl", NED_LTE_MAC_ENB, null);
        createLabel(group, "Grant type / Interactive:");
        createComboIniFieldEditor(group, "*.eNodeB.nic.mac.grantTypeInteractiveUl", NED_LTE_MAC_ENB, "FITALL,URGENT,FIXED", null);
        createLabel(group, "Grant size / Interactive:");
        createTextIniFieldEditor(group, "*.eNodeB.nic.mac.grantSizeInteractiveUl", NED_LTE_MAC_ENB, null);
        createLabel(group, "Grant type / Background:");
        createComboIniFieldEditor(group, "*.eNodeB.nic.mac.grantTypeBackgroundUl", NED_LTE_MAC_ENB, "FITALL,URGENT,FIXED", null);
        createLabel(group, "Grant size / Background:");
        createTextIniFieldEditor(group, "*.eNodeB.nic.mac.grantSizeBackgroundUl", NED_LTE_MAC_ENB, null);

        group = createGroup(sectionBody, "MAC:", 2);
        createLabel(group, "AMC mode:");
        createComboIniFieldEditor(group, "*.eNodeB.nic.mac.amcMode", NED_LTE_MAC_ENB, "AUTO,PILOTED,MULTI,DAS", null);
        createLabel(group, "MU-MIMO min.distance:");
        createTextIniFieldEditor(group, "*.eNodeB.nic.mac.muMimoMinDistance", NED_LTE_MAC_ENB, null);
        createLabel(group, "summaryLowerBound:");
        createTextIniFieldEditor(group, "*.eNodeB.nic.mac.summaryLowerBound", NED_LTE_MAC_ENB, null);
        createLabel(group, "summaryUpperBound:");
        createTextIniFieldEditor(group, "*.eNodeB.nic.mac.summaryUpperBound", NED_LTE_MAC_ENB, null);
        createLabel(group, "fbhbCapacityDl:");
        createTextIniFieldEditor(group, "*.eNodeB.nic.mac.fbhbCapacityDl", NED_LTE_MAC_ENB, null);
        createLabel(group, "fbhbCapacityUl:");
        createTextIniFieldEditor(group, "*.eNodeB.nic.mac.fbhbCapacityUl", NED_LTE_MAC_ENB, null);
        createLabel(group, "pmiWeight:");
        createTextIniFieldEditor(group, "*.eNodeB.nic.mac.pmiWeight", NED_LTE_MAC_ENB, null);
        createLabel(group, "CQI weight:");
        createTextIniFieldEditor(group, "*.eNodeB.nic.mac.cqiWeight", NED_LTE_MAC_ENB, null);
        createLabel(group, "kCqi:");
        createTextIniFieldEditor(group, "*.eNodeB.nic.mac.kCqi", NED_LTE_MAC_ENB, null);
        
        sectionBody = (Composite)createSection(composite, "UEs", 3).getClient();
        Composite comp = createComposite(sectionBody, 1, 1);
        group = createGroup(comp, "General:", 2);
        createLabel(group, "Number of UEs:");
        createTextIniFieldEditor(group, "**.numUe", NED_LTE_NETWORK, null);

        group = createGroup(comp, "Application:", 2);
        ((GridData)group.getLayoutData()).heightHint = 9*approximateLineHeight;
        createLabel(group, "Type:");
        final IFieldEditor ueAppTypeCombo = createComboIniOptionFieldEditor(group, "*.ue*.udpApp[*].typename", "UDPBasicApp,UDPEchoApp,UDPVideoStreamCli,UDPVideoStreamSvr,Gaming,VoDUDPClient,VoDUDPServer,VoIPSender,VoIPReceiver", "UDPVideoStreamCli");
        final Composite group1 = group;
        ueAppTypeCombo.addModifyListener(new ModifyListener() {
            @Override
            public void modifyText(ModifyEvent e) {
                String value = ((Combo)ueAppTypeCombo.getControlForLayouting()).getText();
                replaceParamFieldEditors(group1, ueAppParamsFieldEditors, ueAppParamsExtraControls, value, "*.ue*.udpApp[*]");
            }
        });
        createLabel(group, "Parameters:", 2);
        // here come the dynamically created, type-dependent fields
        
        group = createGroup(sectionBody, "Mobility:", 2);
        ((GridData)group.getLayoutData()).heightHint = 10*approximateLineHeight;
        createLabel(group, "Type:");
        final IFieldEditor ueMobilityTypeCombo = createComboIniFieldEditor(group, "*.ue*.mobilityType", NED_LTE_UE, "StationaryMobility,LinearMobility,RandomWPMobility", null);
        final Composite group2 = group;
        ueMobilityTypeCombo.addModifyListener(new ModifyListener() {
            @Override
            public void modifyText(ModifyEvent e) {
                String value = ((Combo)ueMobilityTypeCombo.getControlForLayouting()).getText();
                replaceParamFieldEditors(group2, ueMobilityParamsFieldEditors, ueMobilityParamsExtraControls, value, "*.ue*.mobility");
            }
        });

        group = createGroup(sectionBody, "Feedback generator:", 2);
        createLabel(group, "RB allocation type:");
        createComboIniFieldEditor(group, "**.rbAllocationType", NED_LTE_FEEDBACKGENERATOR, "distributed,localized", null); // we use ** here because eNodeB's mac and UE's FeedbackGenerator both contain sthis parameter
        createLabel(group, "Feedback type:");
        createComboIniFieldEditor(group, "**.dlFbGen.feedbackType", NED_LTE_FEEDBACKGENERATOR, "ALLBANDS,PREFERRED,WIDEBAND", null);
        createLabel(group, "fbPeriod:");
        createSpinnerIniFieldEditor(group, "**.dlFbGen.fbPeriod", NED_LTE_FEEDBACKGENERATOR, null);
        createLabel(group, "fbDelay:");
        createSpinnerIniFieldEditor(group, "**.dlFbGen.fbDelay", NED_LTE_FEEDBACKGENERATOR, null);
        createLabel(group, "usePeriodic:");
        createBoolIniFieldEditor(group, "**.dlFbGen.usePeriodic", NED_LTE_FEEDBACKGENERATOR, null);
        createLabel(group, "initialTxMode:");
        createComboIniFieldEditor(group, "**.dlFbGen.initialTxMode", NED_LTE_FEEDBACKGENERATOR, "SINGLE_ANTENNA_PORT0,SINGLE_ANTENNA_PORT5,TRANSMIT_DIVERSITY,OL_SPATIAL_MULTIPLEXING,CL_SPATIAL_MULTIPLEXING,MULTI_USER",null);
        createLabel(group, "feedbackGeneratorType:");
        createComboIniFieldEditor(group, "**.dlFbGen.feedbackGeneratorType", NED_LTE_FEEDBACKGENERATOR, "IDEAL,REAL,DAS_AWARE", null);

        sectionBody = (Composite)createSection(composite, "Server (on the internet)", 3).getClient();
        group = createGroup(sectionBody, "Application:", 2);
        ((GridData)group.getLayoutData()).heightHint = 9*approximateLineHeight;
        createLabel(group, "Type:");
        final IFieldEditor serverAppTypeCombo = createComboIniOptionFieldEditor(group, "*.server.udpApp[*].typename", "UDPBasicApp,UDPEchoApp,UDPVideoStreamCli,UDPVideoStreamSvr,Gaming,VoDUDPClient,VoDUDPServer,VoIPSender,VoIPReceiver", "UDPVideoStreamSvr");
        final Composite group3 = group;
        serverAppTypeCombo.addModifyListener(new ModifyListener() {
            @Override
            public void modifyText(ModifyEvent e) {
                String value = ((Combo)serverAppTypeCombo.getControlForLayouting()).getText();
                replaceParamFieldEditors(group3, serverAppParamsFieldEditors, serverAppParamsExtraControls, value, "*.server.udpApp[*]");
            }
        });
        
        createComposite(sectionBody, 1, 1);
        
// TODO: noncofigurble INI file entry needed (e.g. Constraint area Z, typename etc.)          
// TODO: some fields should be checkbox etc; use scrolling pane; revise parameters 
    }

    private int measureDefaultTextControlHeight(Composite composite) {
        Text tmp = new Text(composite, SWT.BORDER);
        Point sz = tmp.computeSize(-1, -1);
        tmp.dispose();
        int defaultTextControlHeight = sz.y;
        return defaultTextControlHeight;
    }

    protected void replaceParamFieldEditors(Composite parent, List<IFieldEditor> panelFieldEditors, List<Control> panelExtraControls, String nedType, String modulePathPattern) {
        
        // dispose existing editors
        for (IFieldEditor e : panelFieldEditors) {
            fieldEditors.remove(e);
            e.dispose();
        }
        for (Control c: panelExtraControls) {
            c.dispose();
        }
        
        panelFieldEditors.clear();
        panelExtraControls.clear();
        
        // add new ones
        if (nedType.equals("UDPBasicApp")) {
            panelExtraControls.add(createLabel(parent, "Local port:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".localPort", NED_UDP_BASIC_APP, null));
            panelExtraControls.add(createLabel(parent, "Destination addresses:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".destAddresses", NED_UDP_BASIC_APP, null));
            panelExtraControls.add(createLabel(parent, "Destination port:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".destPort", NED_UDP_BASIC_APP, null));
            panelExtraControls.add(createLabel(parent, "Packet length:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".messageLength", NED_UDP_BASIC_APP, null));
            panelExtraControls.add(createLabel(parent, "Start time"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".startTime", NED_UDP_BASIC_APP, null));
            panelExtraControls.add(createLabel(parent, "Stop time"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".stopTime", NED_UDP_BASIC_APP, null));
            panelExtraControls.add(createLabel(parent, "Send interval"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".sendInterval", NED_UDP_BASIC_APP, null));
        }
        else if (nedType.equals("Gaming")) {
            panelExtraControls.add(createLabel(parent, "Local port:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".localPort", NED_LTE_APPS_GAMING, null));
            panelExtraControls.add(createLabel(parent, "Destination addresses:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".destAddresses", NED_LTE_APPS_GAMING, null));
            panelExtraControls.add(createLabel(parent, "Destination port:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".destPort", NED_LTE_APPS_GAMING, null));
            panelExtraControls.add(createLabel(parent, "A1:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".a1", NED_LTE_APPS_GAMING, null));
            panelExtraControls.add(createLabel(parent, "B1:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".b1", NED_LTE_APPS_GAMING, null));
            panelExtraControls.add(createLabel(parent, "A2:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".a2", NED_LTE_APPS_GAMING, null));
            panelExtraControls.add(createLabel(parent, "B2:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".b2", NED_LTE_APPS_GAMING, null));
        }
        else if (nedType.equals("VoDUDPClient")) {
            panelExtraControls.add(createLabel(parent, "Local port:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".localPort", NED_LTE_APPS_VODUDPCLIENT, null));
            panelExtraControls.add(createLabel(parent, "startStreamTime:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".startStreamTime", NED_LTE_APPS_VODUDPCLIENT, null));
            panelExtraControls.add(createLabel(parent, "startMetrics:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".startMetrics", NED_LTE_APPS_VODUDPCLIENT, null));
            panelExtraControls.add(createLabel(parent, "vod_trace_file:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".vod_trace_file", NED_LTE_APPS_VODUDPCLIENT, null));
            panelExtraControls.add(createLabel(parent, "bsePath:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".bsePath", NED_LTE_APPS_VODUDPCLIENT, null));
            panelExtraControls.add(createLabel(parent, "origVideoYuv:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".origVideoYuv", NED_LTE_APPS_VODUDPCLIENT, null));
            panelExtraControls.add(createLabel(parent, "origVideoSvc:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".origVideoSvc", NED_LTE_APPS_VODUDPCLIENT, null));
            panelExtraControls.add(createLabel(parent, "decPath:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".decPath", NED_LTE_APPS_VODUDPCLIENT, null));
            panelExtraControls.add(createLabel(parent, "traceType:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".traceType", NED_LTE_APPS_VODUDPCLIENT, null));
            panelExtraControls.add(createLabel(parent, "playbackSize:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".playbackSize", NED_LTE_APPS_VODUDPCLIENT, null));
            panelExtraControls.add(createLabel(parent, "avipluginPath:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".avipluginPath", NED_LTE_APPS_VODUDPCLIENT, null));
            panelExtraControls.add(createLabel(parent, "numFrame:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".numFrame", NED_LTE_APPS_VODUDPCLIENT, null));
            panelExtraControls.add(createLabel(parent, "numPktPerFrame:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".numPktPerFrame", NED_LTE_APPS_VODUDPCLIENT, null));
        }
        else if (nedType.equals("VoDUDPServer")) {
            panelExtraControls.add(createLabel(parent, "Local port:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".localPort", NED_LTE_APPS_VODUDPSERVER, null));
            panelExtraControls.add(createLabel(parent, "Destination addresses:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".destAddresses", NED_LTE_APPS_VODUDPSERVER, null));
            panelExtraControls.add(createLabel(parent, "Destination port:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".destPort", NED_LTE_APPS_VODUDPSERVER, null));
            panelExtraControls.add(createLabel(parent, "vod_trace_file:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".vod_trace_file", NED_LTE_APPS_VODUDPSERVER, null));
            panelExtraControls.add(createLabel(parent, "fps:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".fps", NED_LTE_APPS_VODUDPSERVER, null));
            panelExtraControls.add(createLabel(parent, "traceType:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".traceType", NED_LTE_APPS_VODUDPSERVER, null));
            panelExtraControls.add(createLabel(parent, "clientsStartStreamTime:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".clientsStartStreamTime", NED_LTE_APPS_VODUDPSERVER, null));
            panelExtraControls.add(createLabel(parent, "clientsReqTime:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".clientsReqTime", NED_LTE_APPS_VODUDPSERVER, null));
            panelExtraControls.add(createLabel(parent, "startTime:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".startTime", NED_LTE_APPS_VODUDPSERVER, null));
        }
        else if (nedType.equals("VoIPReceiver")) {
            panelExtraControls.add(createLabel(parent, "Local port:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".localPort", NED_LTE_APPS_VOIP_VOIPRECEIVER, null));
            panelExtraControls.add(createLabel(parent, "emodel_Ie_:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".emodel_Ie_", NED_LTE_APPS_VOIP_VOIPRECEIVER, null));
            panelExtraControls.add(createLabel(parent, "emodel_Bpl_:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".emodel_Bpl_", NED_LTE_APPS_VOIP_VOIPRECEIVER, null));
            panelExtraControls.add(createLabel(parent, "emodel_A_:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".emodel_A_", NED_LTE_APPS_VOIP_VOIPRECEIVER, null));
            panelExtraControls.add(createLabel(parent, "emodel_Ro_:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".emodel_Ro_", NED_LTE_APPS_VOIP_VOIPRECEIVER, null));
            panelExtraControls.add(createLabel(parent, "playout_delay:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".playout_delay", NED_LTE_APPS_VOIP_VOIPRECEIVER, null));
            panelExtraControls.add(createLabel(parent, "dim_buffer:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".dim_buffer", NED_LTE_APPS_VOIP_VOIPRECEIVER, null));
            panelExtraControls.add(createLabel(parent, "Sampling time:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".sampling_time", NED_LTE_APPS_VOIP_VOIPRECEIVER, null));
        }
        else if (nedType.equals("VoIPSender")) {
            panelExtraControls.add(createLabel(parent, "Local port:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".localPort", NED_LTE_APPS_VOIP_VOIPSENDER, null));
            panelExtraControls.add(createLabel(parent, "Destination address:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".destAddress", NED_LTE_APPS_VOIP_VOIPSENDER, null));
            panelExtraControls.add(createLabel(parent, "Destination port:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".destPort", NED_LTE_APPS_VOIP_VOIPSENDER, null));
            panelExtraControls.add(createLabel(parent, "PacketSize:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".PacketSize", NED_LTE_APPS_VOIP_VOIPSENDER, null));
            panelExtraControls.add(createLabel(parent, "shape_talk:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".shape_talk", NED_LTE_APPS_VOIP_VOIPSENDER, null));
            panelExtraControls.add(createLabel(parent, "scale_talk:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".scale_talk", NED_LTE_APPS_VOIP_VOIPSENDER, null));
            panelExtraControls.add(createLabel(parent, "shape_sil:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".shape_sil", NED_LTE_APPS_VOIP_VOIPSENDER, null));
            panelExtraControls.add(createLabel(parent, "scale_sil:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".scale_sil", NED_LTE_APPS_VOIP_VOIPSENDER, null));
            panelExtraControls.add(createLabel(parent, "is_talk:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".is_talk", NED_LTE_APPS_VOIP_VOIPSENDER, null));
            panelExtraControls.add(createLabel(parent, "sampling_time:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".sampling_time", NED_LTE_APPS_VOIP_VOIPSENDER, null));
            panelExtraControls.add(createLabel(parent, "startTime:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".startTime", NED_LTE_APPS_VOIP_VOIPSENDER, null));
        }
        else if (nedType.equals("UDPEchoApp")) {
            panelExtraControls.add(createLabel(parent, "Local port:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".localPort", NED_UDP_ECHO_APP, null));
        }
        else if (nedType.equals("UDPVideoStreamCli")) {
            panelExtraControls.add(createLabel(parent, "Local port:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".localPort", NED_UDP_VIDEO_CLI_APP, "9999"));
            panelExtraControls.add(createLabel(parent, "Server address:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".serverAddress", NED_UDP_VIDEO_CLI_APP, "\"server\""));
            panelExtraControls.add(createLabel(parent, "Server port:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".serverPort", NED_UDP_VIDEO_CLI_APP, "3088"));
            panelExtraControls.add(createLabel(parent, "Start time"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".startTime", NED_UDP_VIDEO_CLI_APP, null));
        }
        else if (nedType.equals("UDPVideoStreamSvr")) {
            panelExtraControls.add(createLabel(parent, "Local port:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".localPort", NED_UDP_VIDEO_SVR_APP, "3088"));
            panelExtraControls.add(createLabel(parent, "Send interval"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".sendInterval", NED_UDP_VIDEO_SVR_APP, "2ms"));
            panelExtraControls.add(createLabel(parent, "Packet length:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".packetLen", NED_UDP_VIDEO_SVR_APP, "1000B"));
            panelExtraControls.add(createLabel(parent, "Video size:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".videoSize", NED_UDP_VIDEO_SVR_APP, "100MiB"));
        }
        else if (nedType.equals("StationaryMobility")) {
            //Note: we use initFromDisplayString=true, so no use for initialX/initialY
            panelExtraControls.add(createLabel(parent, "Initial X:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".initialX", NED_STATIONARY_MOBILITY, "uniform(0m,500m)"));
            panelExtraControls.add(createLabel(parent, "Initial Y:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".initialY", NED_STATIONARY_MOBILITY, "uniform(0m,500m)"));

            panelExtraControls.add(createLabel(parent, "Constraint area min X:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".constraintAreaMinX", NED_STATIONARY_MOBILITY, "0m"));
            panelExtraControls.add(createLabel(parent, "Constraint area max X:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".constraintAreaMaxX", NED_STATIONARY_MOBILITY, "500m"));
            panelExtraControls.add(createLabel(parent, "Constraint area min Y:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".constraintAreaMinY", NED_STATIONARY_MOBILITY, "0m"));
            panelExtraControls.add(createLabel(parent, "Constraint area max Y:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".constraintAreaMaxY", NED_STATIONARY_MOBILITY, "500m"));
        }
        else if (nedType.equals("LinearMobility")) {
            panelExtraControls.add(createLabel(parent, "Initial X:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".initialX", NED_LINEAR_MOBILITY, "uniform(0m,500m)"));
            panelExtraControls.add(createLabel(parent, "Initial Y:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".initialY", NED_LINEAR_MOBILITY, "uniform(0m,500m)"));
            panelExtraControls.add(createLabel(parent, "Speed:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".speed", NED_LINEAR_MOBILITY, null));
            panelExtraControls.add(createLabel(parent, "Angle:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".angle", NED_LINEAR_MOBILITY, null));
            panelExtraControls.add(createLabel(parent, "Acceleration:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".acceleration", NED_LINEAR_MOBILITY, null));

            //TODO this is copy/pasta from above
            panelExtraControls.add(createLabel(parent, "Constraint area min X:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".constraintAreaMinX", NED_STATIONARY_MOBILITY, "0m"));
            panelExtraControls.add(createLabel(parent, "Constraint area max X:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".constraintAreaMaxX", NED_STATIONARY_MOBILITY, "500m"));
            panelExtraControls.add(createLabel(parent, "Constraint area min Y:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".constraintAreaMinY", NED_STATIONARY_MOBILITY, "0m"));
            panelExtraControls.add(createLabel(parent, "Constraint area max Y:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".constraintAreaMaxY", NED_STATIONARY_MOBILITY, "500m"));
        }
        else if (nedType.equals("RandomWPMobility")) {
            panelExtraControls.add(createLabel(parent, "Initial X:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".initialX", NED_RANDOMWP_MOBILITY, "uniform(0m,500m)"));
            panelExtraControls.add(createLabel(parent, "Initial Y:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".initialY", NED_RANDOMWP_MOBILITY, "uniform(0m,500m)"));
            panelExtraControls.add(createLabel(parent, "Speed:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".speed", NED_RANDOMWP_MOBILITY, null));
            panelExtraControls.add(createLabel(parent, "Wait time:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".waitTime", NED_RANDOMWP_MOBILITY, null));
            
            //TODO this is copy/pasta from above
            panelExtraControls.add(createLabel(parent, "Constraint area min X:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".constraintAreaMinX", NED_STATIONARY_MOBILITY, "0m"));
            panelExtraControls.add(createLabel(parent, "Constraint area max X:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".constraintAreaMaxX", NED_STATIONARY_MOBILITY, "500m"));
            panelExtraControls.add(createLabel(parent, "Constraint area min Y:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".constraintAreaMinY", NED_STATIONARY_MOBILITY, "0m"));
            panelExtraControls.add(createLabel(parent, "Constraint area max Y:"));
            panelFieldEditors.add(createTextIniFieldEditor(parent, modulePathPattern + ".constraintAreaMaxY", NED_STATIONARY_MOBILITY, "500m"));
        }
        else if (nedType.equals("")) {
            // leave empty
        }
        else {
            throw new RuntimeException("unknown app NED type " + nedType);
        }

        if (populateControlsDone) // else populateControls() will initialize our new editors too
            initializeFieldEditors(); // will do all uninitialized ones

        parent.layout(true); // IMPORTANT: parent MUST already be large enough to accommodate field editors above
    }

    // ini parameter editor versions

    protected IFieldEditor createTextIniFieldEditor(Composite parent, String iniKey, String ownerNedType, String defaultValue) {
        return add(new TextFieldEditor(createText(parent), new IniParameterPropertySource(document, iniKey, ownerNedType, defaultValue), hoverSupport));
    }

    protected IFieldEditor createBoolIniFieldEditor(Composite parent, String iniKey, String ownerNedType, String defaultValue) {
        return createComboIniFieldEditor(parent, iniKey, ownerNedType, "true,false", defaultValue);
    }

    protected IFieldEditor createComboIniFieldEditor(Composite parent, String iniKey, String ownerNedType, String items, String defaultValue) {
        return add(new ComboFieldEditor(createCombo(parent, items), new IniParameterPropertySource(document, iniKey, ownerNedType, defaultValue), hoverSupport));
    }

    protected IFieldEditor createSpinnerIniFieldEditor(Composite parent, String iniKey, String ownerNedType, String defaultValue) {
        return add(new SpinnerFieldEditor(createSpinner(parent, 0, 9999), new IniParameterPropertySource(document, iniKey, ownerNedType, null), hoverSupport));
    }

    protected IFieldEditor createComboIniOptionFieldEditor(Composite parent, String iniKey, String items, String defaultValue) {
        return add(new ComboFieldEditor(createCombo(parent, items), new IniConfigOptionPropertySource(document, iniKey, defaultValue), hoverSupport)); // for editing an option such as **.typename
    }

    protected IFieldEditor createTextIniOptionEditor(Composite parent, String iniKey, String defaultValue) {
        return add(new TextFieldEditor(createText(parent), new IniConfigOptionPropertySource(document, iniKey, defaultValue), hoverSupport));
    }


    protected ParsedXmlIniParameter createParsedXmlIniParameter(String iniKey, String ownerNedType) {
        return createParsedXmlIniParameter(iniKey, ownerNedType, null);
    }
    
    protected ParsedXmlIniParameter createParsedXmlIniParameter(String iniKey, String ownerNedType, String defaultValue) {
        ParsedXmlIniParameter p = new ParsedXmlIniParameter(new IniParameterPropertySource(document, iniKey, ownerNedType, defaultValue));
        xmlParameters.add(p);
        return p;
    }

    // xml attribute versions

    protected IFieldEditor createTextXmlAttrEditor(Composite parent, ParsedXmlIniParameter xml, IXmlAttributeProvider attrProvider, String defaultValue) {
        return add(new TextFieldEditor(createText(parent), new XmlAttributePropertySource(xml, attrProvider, defaultValue), hoverSupport));
    }

    protected IFieldEditor createBoolXmlAttrEditor(Composite parent, ParsedXmlIniParameter xml, IXmlAttributeProvider attrProvider, boolean defaultValue) {
        return createComboXmlAttrEditor(parent, xml, attrProvider, "true,false", defaultValue ? "true" : "false");
    }

    protected IFieldEditor createComboXmlAttrEditor(Composite parent, ParsedXmlIniParameter xml, IXmlAttributeProvider attrProvider, String items, String defaultValue) {
        return add(new ComboFieldEditor(createCombo(parent, items), new XmlAttributePropertySource(xml, attrProvider, defaultValue), hoverSupport));
    }

    protected IFieldEditor createSpinnerXmlAttrEditor(Composite parent, ParsedXmlIniParameter xml, IXmlAttributeProvider attrProvider, String defaultValue) {
        return add(new SpinnerFieldEditor(createSpinner(parent, 0, 9999), new XmlAttributePropertySource(xml, attrProvider, defaultValue), hoverSupport));
    }

    // multi-property-source versions
    
    protected IFieldEditor createTextXmlAttrEditor(Composite parent, IPropertySource[] propertySources) {
        return add(new TextFieldEditor(createText(parent), new MultiPropertySource(propertySources), hoverSupport));
    }

    protected IFieldEditor createBoolXmlAttrEditor(Composite parent, IPropertySource[] propertySources) {
        return createComboXmlAttrEditor(parent, propertySources, "true,false");
    }

    protected IFieldEditor createComboXmlAttrEditor(Composite parent, IPropertySource[] propertySources, String items) {
        return add(new ComboFieldEditor(createCombo(parent, items), new MultiPropertySource(propertySources), hoverSupport));
    }

    protected IFieldEditor createSpinnerXmlAttrEditor(Composite parent, IPropertySource[] propertySources) {
        return add(new SpinnerFieldEditor(createSpinner(parent, 0, 9999), new MultiPropertySource(propertySources), hoverSupport));
    }

    
    protected IFieldEditor add(IFieldEditor fieldEditor) {
        fieldEditors.add(fieldEditor);
        fieldEditor.addModifyListener(modifyListener);
        return fieldEditor;
    }

    protected Section createSection(Composite parent, String label, int ncols) {
        Section section = toolkit.createSection(parent, Section.TITLE_BAR | Section.EXPANDED | Section.TREE_NODE);
        section.setText(label);
        section.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
        Composite sectionBody = toolkit.createComposite(section);
        section.setClient(sectionBody);
        sectionBody.setLayout(new GridLayout(ncols, false));
        return section;
    }

    protected Composite createComposite(Composite parent, int hspan, int ncols) {
        Composite composite = new Composite(parent, SWT.NONE);
        toolkit.adapt(composite);
        GridData gridData = new GridData(SWT.FILL, SWT.FILL, true, false);
        gridData.horizontalSpan = hspan;
        composite.setLayoutData(gridData);
        GridLayout layout = new GridLayout(ncols, false);
        layout.marginWidth = layout.marginHeight = 0;
        composite.setLayout(layout);
        return composite;
    }

    protected Group createGroup(Composite parent, String label, int ncols) {
        Group group = new Group(parent, SWT.NONE);
        toolkit.adapt(group);
        group.setText(label);
        group.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
        group.setLayout(new GridLayout(ncols, false));
        return group;
    }

    protected Label createLabel(Composite parent, String name) {
        return createLabel(parent, name, 1);
    }
    
    protected Label createLabel(Composite parent, String name, int hspan) {
        Label label = toolkit.createLabel(parent, name, SWT.NONE);
        label.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, false, false, hspan, 1));
        return label;
    }

    protected Label createWrappingLabel(Composite parent, String text, int hspan) {
        Label label = toolkit.createLabel(parent, text, SWT.WRAP);
        GridData gridData = new GridData(SWT.FILL, SWT.CENTER, true, false, hspan, 1);
        gridData.widthHint = 500; // cannot be too small, otherwise label will request a large height
        label.setLayoutData(gridData);
        return label;
    }

    protected Spinner createSpinner(Composite parent, int minValue, int maxValue) {
        Spinner spinner = new Spinner(parent, SWT.BORDER);
        spinner.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
        spinner.setMinimum(minValue);
        spinner.setMaximum(maxValue);
        return spinner;
    }

    protected Text createText(Composite parent) {
        Text text = new Text(parent, SWT.BORDER);
        text.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
        return text;
    }

    protected Combo createCombo(Composite parent) {
        Combo combo = new Combo(parent, SWT.SINGLE | SWT.BORDER | SWT.READ_ONLY);
        GridData layoutData = new GridData(SWT.FILL, SWT.FILL, true, false);
        layoutData.widthHint = 40;
        combo.setLayoutData(layoutData);
        return combo;
    }

    protected Combo createCombo(Composite parent, String items) {
        Combo combo = createCombo(parent);
        for (String i : items.split(","))
            combo.add(i);
        if (items.endsWith(","))
            combo.add("");
        return combo;
    }

    protected Combo createEditableCombo(Composite parent) {
        Combo combo = new Combo(parent, SWT.SINGLE | SWT.BORDER);
        combo.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, false));
        return combo;
    }

    @Override
    public void setFocus() {
        fieldEditors.get(0).getControlForLayouting().setFocus();
    }

    protected void populateControls() {
        INedResources nedResources = NedResourcesPlugin.getNedResources();
        if (nedResources.isLoadingInProgress()) {
            // try again later
            Display.getCurrent().asyncExec(new Runnable() {
                @Override
                public void run() {
                    populateControls();
                }
            });
            return;
        }

        // parse XML attributes
        for (ParsedXmlIniParameter e : xmlParameters)
            e.parse();
            
        initializeFieldEditors();

        setDirty(false);
        
        populateControlsDone = true;
    }

    protected void initializeFieldEditors() {
        // Note that initializing one editor might cause new editors to spring into existence 
        // (via ModifyListeners), so we have to do initialization in multiple passes
        boolean again;
        do {        
            again = false;
            for (IFieldEditor e : fieldEditors.toArray(new IFieldEditor[]{})) {
                if (!e.isInitialized()) {
                    e.initialize();
                    again = true;
                }
            }
        } while (again);
    }

    @Override
    public void doSave(IProgressMonitor monitor) {
        applyControls();
        super.doSave(monitor);
    }
    
    protected void applyControls() {
        // commit all field editors
        for (IFieldEditor e : fieldEditors)
            e.commit();

        for (ParsedXmlIniParameter e : xmlParameters)
            e.unparse();
    }
    
    protected static Attr getAttr(Element parent, String attrName) {
        return parent.getAttributeNode(attrName);
    }

    protected static Attr getOrCreateAttr(Element parent, String attrName) {
        Attr e = getAttr(parent, attrName);
        if (e == null) {
            parent.setAttribute(attrName, "");
            e = getAttr(parent, attrName);
            Assert.isNotNull(e);
        }
        return e;
    }
    
    protected static Element getOrCreateFirstChildByTagName(Element parent, String tagName) {
        Element e = getFirstChildByTagName(parent, tagName);
        if (e == null) {
            e = parent.getOwnerDocument().createElement(tagName);
            parent.appendChild(e);
        }
        return e;
    }

    protected static Element getOrCreateFirstChildByTagName(Element parent, String tagName, String attrName, String value) {
        Element e = getFirstChildByTagName(parent, tagName, attrName, value);
        if (e == null) {
            e = parent.getOwnerDocument().createElement(tagName);
            parent.appendChild(e);
            e.setAttribute(attrName, value);
        }
        return e;
    }
    
    protected static Element getFirstChildByTagName(Element parent, String tagName) {
        for (Node e = parent.getFirstChild(); e != null; e = e.getNextSibling())
            if (e instanceof Element && ((Element)e).getTagName().equals(tagName))
                return (Element)e;
        return null;
    }

    protected static Element getFirstChildByTagName(Element parent, String tagName, String attrName, String value) {
        for (Node e = parent.getFirstChild(); e != null; e = e.getNextSibling())
            if (e instanceof Element && ((Element)e).getTagName().equals(tagName) && 
                    ObjectUtils.equals(((Element)e).getAttribute(attrName), value))
                return (Element)e;
        return null;
    }
}

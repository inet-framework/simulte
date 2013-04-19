/**
 * Copyright (c) 2012 Opensim Ltd. All rights reserved.
 */

package com.simulte.configeditor.editors;

import java.io.IOException;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;

import org.eclipse.core.resources.IFile;
import org.eclipse.core.runtime.CoreException;
import org.omnetpp.inifile.editor.model.InifileParser;
import org.omnetpp.inifile.editor.model.ParseException;

/**
 * Represents the contents of an inifile as key-value pairs, and provides parse,
 * unparse, get and set operations. The order of entries, comments etc. are not
 * preserved. Multiple sections, includes, and other fancy stuff cannot be
 * represented and thus are rejected by the parser.
 * 
 * @author Andras
 */
public class SimplifiedInifileDocument {
    private IFile file;
    private Map<String,String> keyValueMap = new HashMap<String, String>();
    
    private class Callback implements InifileParser.ParserCallback {
        @Override
        public void blankOrCommentLine(int lineNumber, int numLines, String rawLine, String rawComment) {
            // ignore
        }

        @Override
        public void sectionHeadingLine(int lineNumber, int numLines, String rawLine, String sectionName, String rawComment) {
            if (!sectionName.equals("General"))
                parseError(lineNumber, numLines, "Only the [General] section is supported");
        }

        @Override
        public void keyValueLine(int lineNumber, int numLines, String rawLine, String key, String value, String rawComment) {
            keyValueMap.put(key, value);
        }

        @Override
        public void directiveLine(int lineNumber, int numLines, String rawLine, String directive, String args, String rawComment) {
            parseError(lineNumber, numLines, "Directive lines such as \"include\" are not supported");
        }

        @Override
        public void parseError(int lineNumber, int numLines, String message) {
            throw new ParseException(message, lineNumber);
        }
        
    }

    public SimplifiedInifileDocument() {
    }
    
    public SimplifiedInifileDocument(IFile file) {
        this.file = file;
    }
    
    public void parse() throws ParseException, CoreException, IOException {
        if (file == null)
            throw new RuntimeException("no file given");
        new InifileParser().parse(file, new Callback());
    }
    
    public IFile getDocumentFile() {
        return file;
    }
    
    public void setValue(String key, String value) {
        keyValueMap.put(key, value);
    }

    public String getValue(String key) {
        return keyValueMap.get(key);
    }

    public void removeValue(String key) {
        keyValueMap.remove(key);
    }

    public String getContents() throws CoreException {
        String[] keys = keyValueMap.keySet().toArray(new String[]{});
        Arrays.sort(keys);

        StringBuilder result = new StringBuilder();
        result.append("[General]\n");
        for (String key : keys) {
            result.append(key);
            result.append(" = ");
            result.append(keyValueMap.get(key));
            result.append("\n");
        }
        return result.toString();
    }
    
    public String toString() {
        String[] keys = keyValueMap.keySet().toArray(new String[]{});
        Arrays.sort(keys);

        StringBuilder result = new StringBuilder();
        for (String key : keys) {
            result.append(key);
            result.append(" = ");
            result.append(keyValueMap.get(key));
            result.append("\n");
        }
        return result.toString();
    }
}

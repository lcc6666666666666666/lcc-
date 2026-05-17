import { useEffect, useRef } from 'react';
import MonacoEditor from '@monaco-editor/react';
import type * as Monaco from 'monaco-editor';
import { registerSnlLanguage } from '../snlLanguage';
import type { Diagnostic } from '../types';

interface EditorPaneProps {
  source: string;
  diagnostics: Diagnostic[];
  onChange: (value: string) => void;
  onCursorChange: (line: number, column: number) => void;
  onReady: (editor: Monaco.editor.IStandaloneCodeEditor, monaco: typeof Monaco) => void;
  onCompile: () => void;
}

export default function EditorPane({
  source,
  diagnostics,
  onChange,
  onCursorChange,
  onReady,
  onCompile,
}: EditorPaneProps) {
  const editorRef = useRef<Monaco.editor.IStandaloneCodeEditor | null>(null);
  const monacoRef = useRef<typeof Monaco | null>(null);
  const onCompileRef = useRef(onCompile);

  useEffect(() => {
    onCompileRef.current = onCompile;
  }, [onCompile]);

  useEffect(() => {
    const editor = editorRef.current;
    const monaco = monacoRef.current;
    const model = editor?.getModel();
    if (!editor || !monaco || !model) {
      return;
    }

    const markers = diagnostics.map((diagnostic) => ({
      startLineNumber: Math.max(1, diagnostic.line || 1),
      startColumn: Math.max(1, diagnostic.column || 1),
      endLineNumber: Math.max(1, diagnostic.line || 1),
      endColumn: Math.max(2, (diagnostic.column || 1) + 1),
      message: diagnostic.message,
      severity: markerSeverity(monaco, diagnostic.severity),
      source: diagnostic.stage,
    }));

    monaco.editor.setModelMarkers(model, 'snl-compiler', markers);
  }, [diagnostics]);

  return (
    <div className="editor-pane">
      <MonacoEditor
        value={source}
        language="snl"
        theme="snl-dark"
        beforeMount={(monaco) => registerSnlLanguage(monaco)}
        onChange={(value) => onChange(value ?? '')}
        onMount={(editor, monaco) => {
          editorRef.current = editor;
          monacoRef.current = monaco;
          onReady(editor, monaco);
          const position = editor.getPosition();
          if (position) {
            onCursorChange(position.lineNumber, position.column);
          }
          editor.onDidChangeCursorPosition((event) => {
            onCursorChange(event.position.lineNumber, event.position.column);
          });
          editor.addCommand(monaco.KeyMod.CtrlCmd | monaco.KeyCode.Enter, () => onCompileRef.current());
        }}
        options={{
          automaticLayout: true,
          fontFamily: '"Cascadia Code", Consolas, "Courier New", monospace',
          fontSize: 14,
          minimap: { enabled: true },
          renderLineHighlight: 'all',
          scrollBeyondLastLine: false,
          tabSize: 2,
          wordWrap: 'on',
        }}
      />
    </div>
  );
}

function markerSeverity(monaco: typeof Monaco, severity: Diagnostic['severity']) {
  if (severity === 'warning') {
    return monaco.MarkerSeverity.Warning;
  }
  if (severity === 'info') {
    return monaco.MarkerSeverity.Info;
  }
  return monaco.MarkerSeverity.Error;
}

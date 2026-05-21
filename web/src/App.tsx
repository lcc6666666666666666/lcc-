import { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import type * as Monaco from 'monaco-editor';
import { compileSource } from './api';
import ActivityBar from './components/ActivityBar';
import BottomPanel from './components/BottomPanel';
import CompileToolbar from './components/CompileToolbar';
import EditorPane from './components/EditorPane';
import { parseAstTree } from './components/AstTree';
import { parseTokens } from './components/TokenTable';
import type { BottomTab, CompileResponse, CompileStatus, CompilerOutputs, SourceFileKey } from './types';

const STORAGE_KEY = 'snl-web-ide-source';

const EXAMPLE_SOURCE = `program demo
var
  integer x,y;
begin
  read(x);
  y:=x+1;
  if y<10
  then
    write(y)
  else
    write(x)
  fi
end.`;

const EMPTY_OUTPUTS: CompilerOutputs = {
  token: '',
  tree: '',
  midcode: '',
  mips: '',
  error: '',
  stdout: '',
  stderr: '',
};

const EMPTY_RESULT: CompileResponse = {
  ok: true,
  durationMs: 0,
  outputs: EMPTY_OUTPUTS,
  diagnostics: [],
};

export default function App() {
  const [source, setSource] = useState(() => localStorage.getItem(STORAGE_KEY) ?? EXAMPLE_SOURCE);
  const [result, setResult] = useState<CompileResponse>(EMPTY_RESULT);
  const [status, setStatus] = useState<CompileStatus>('idle');
  const [activeFile, setActiveFile] = useState<SourceFileKey>('source');
  const [activeTab, setActiveTab] = useState<BottomTab>('problems');
  const [rawFile, setRawFile] = useState<SourceFileKey>('token');
  const [cursor, setCursor] = useState({ line: 1, column: 1 });
  const editorRef = useRef<Monaco.editor.IStandaloneCodeEditor | null>(null);
  const monacoRef = useRef<typeof Monaco | null>(null);

  const tokenRows = useMemo(() => parseTokens(result.outputs.token), [result.outputs.token]);
  const astNodes = useMemo(() => parseAstTree(result.outputs.tree), [result.outputs.tree]);

  useEffect(() => {
    localStorage.setItem(STORAGE_KEY, source);
  }, [source]);

  const jumpToLine = useCallback((line: number) => {
    const editor = editorRef.current;
    if (!editor) {
      return;
    }
    editor.focus();
    editor.setPosition({ lineNumber: Math.max(1, line), column: 1 });
    editor.revealLineInCenter(Math.max(1, line));
  }, []);

  const handleCompile = useCallback(async () => {
    if (status === 'compiling') {
      return;
    }

    setStatus('compiling');
    try {
      const response = await compileSource(source);
      setResult(response);
      const hasErrors = !response.ok || response.diagnostics.length > 0;
      setStatus(hasErrors ? 'error' : 'success');
      setActiveTab(hasErrors ? 'problems' : 'tokens');
      setActiveFile(hasErrors ? 'error' : 'token');
    } catch (error) {
      const message = error instanceof Error ? error.message : 'Compile request failed.';
      setResult({
        ok: false,
        durationMs: 0,
        outputs: { ...EMPTY_OUTPUTS, stderr: message },
        diagnostics: [
          {
            line: 1,
            column: 1,
            message,
            stage: 'compiler',
            severity: 'error',
          },
        ],
        message,
      });
      setStatus('error');
      setActiveTab('problems');
      setActiveFile('error');
    }
  }, [source, status]);

  useEffect(() => {
    const handleKeyDown = (event: KeyboardEvent) => {
      if ((event.ctrlKey || event.metaKey) && event.key === 'Enter') {
        event.preventDefault();
        void handleCompile();
      }
    };

    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [handleCompile]);

  const handleSelectFile = useCallback((file: SourceFileKey) => {
    setActiveFile(file);
    if (file === 'source') {
      editorRef.current?.focus();
      return;
    }
    if (file === 'token') {
      setActiveTab('tokens');
      return;
    }
    if (file === 'tree') {
      setActiveTab('ast');
      return;
    }
    if (file === 'midcode') {
      setActiveTab('midcode');
      return;
    }
    if (file === 'mips') {
      setActiveTab('mips');
      return;
    }
    setRawFile('error');
    setActiveTab('raw');
  }, []);

  const handleClearOutput = useCallback(() => {
    setResult(EMPTY_RESULT);
    setStatus('idle');
    setActiveTab('problems');
    setActiveFile('source');
  }, []);

  const handleResetExample = useCallback(() => {
    setSource(EXAMPLE_SOURCE);
    editorRef.current?.focus();
  }, []);

  const handleCopyMips = useCallback(async () => {
    if (!result.outputs.mips.trim()) {
      return;
    }
    await navigator.clipboard.writeText(result.outputs.mips);
  }, [result.outputs.mips]);

  return (
    <div className="app-shell">
      <CompileToolbar
        status={status}
        onCompile={() => void handleCompile()}
        onResetExample={handleResetExample}
        onClearOutput={handleClearOutput}
      />
      <div className="workbench">
        <ActivityBar activeFile={activeFile} onSelectFile={handleSelectFile} />
        <main className="main-area">
          <EditorPane
            source={source}
            diagnostics={result.diagnostics}
            onChange={setSource}
            onCursorChange={(line, column) => setCursor({ line, column })}
            onReady={(editor, monaco) => {
              editorRef.current = editor;
              monacoRef.current = monaco;
            }}
            onCompile={() => void handleCompile()}
          />
          <BottomPanel
            activeTab={activeTab}
            onSelectTab={setActiveTab}
            outputs={result.outputs}
            diagnostics={result.diagnostics}
            tokenRows={tokenRows}
            astNodes={astNodes}
            rawFile={rawFile}
            onSelectRawFile={setRawFile}
            onSelectLine={jumpToLine}
            onCopyMips={() => void handleCopyMips()}
          />
        </main>
      </div>
      <footer className="status-bar">
        <span>Ln {cursor.line}, Col {cursor.column}</span>
        <span>{source.length} chars</span>
        <span>{tokenRows.length} tokens</span>
        <span>{result.diagnostics.length} diagnostics</span>
        <span>{result.durationMs ? `${result.durationMs} ms` : 'No compile yet'}</span>
        {result.message && <span className="status-bar__message">{result.message}</span>}
      </footer>
    </div>
  );
}

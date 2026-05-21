import type { BottomTab, CompilerOutputs, Diagnostic, SourceFileKey, TokenRow, AstNode } from '../types';
import AstTree from './AstTree';
import ProblemsPanel from './ProblemsPanel';
import ReadonlyCodePane from './ReadonlyCodePane';
import TokenTable from './TokenTable';

interface BottomPanelProps {
  activeTab: BottomTab;
  onSelectTab: (tab: BottomTab) => void;
  outputs: CompilerOutputs;
  diagnostics: Diagnostic[];
  tokenRows: TokenRow[];
  astNodes: AstNode[];
  rawFile: SourceFileKey;
  onSelectRawFile: (file: SourceFileKey) => void;
  onSelectLine: (line: number) => void;
  onCopyMips: () => void;
}

const tabs: Array<{ key: BottomTab; label: string }> = [
  { key: 'problems', label: 'Problems' },
  { key: 'tokens', label: 'Tokens' },
  { key: 'ast', label: 'AST' },
  { key: 'midcode', label: 'Midcode' },
  { key: 'mips', label: 'MIPS' },
  { key: 'console', label: 'Console' },
  { key: 'raw', label: 'Raw Files' },
];

const rawFiles: Array<{ key: SourceFileKey; label: string }> = [
  { key: 'token', label: 'token.txt' },
  { key: 'tree', label: 'tree.txt' },
  { key: 'midcode', label: 'midcode.txt' },
  { key: 'mips', label: 'mips.txt' },
  { key: 'error', label: 'error.txt' },
];

export default function BottomPanel({
  activeTab,
  onSelectTab,
  outputs,
  diagnostics,
  tokenRows,
  astNodes,
  rawFile,
  onSelectRawFile,
  onSelectLine,
  onCopyMips,
}: BottomPanelProps) {
  return (
    <section className="bottom-panel">
      <div className="panel-tabs">
        {tabs.map((tab) => (
          <button
            key={tab.key}
            className={`panel-tab ${activeTab === tab.key ? 'panel-tab--active' : ''}`}
            type="button"
            onClick={() => onSelectTab(tab.key)}
          >
            {tab.label}
            {tab.key === 'problems' && diagnostics.length > 0 && <span className="tab-count">{diagnostics.length}</span>}
          </button>
        ))}
      </div>
      <div className="panel-body">
        {activeTab === 'problems' && <ProblemsPanel diagnostics={diagnostics} onSelectLine={onSelectLine} />}
        {activeTab === 'tokens' && <TokenTable rows={tokenRows} onSelectLine={onSelectLine} />}
        {activeTab === 'ast' && <AstTree nodes={astNodes} rawText={outputs.tree} />}
        {activeTab === 'midcode' && (
          <div className="code-panel">
            <div className="code-panel__editor">
              <ReadonlyCodePane value={outputs.midcode} language="plaintext" emptyText="No midcode output yet" />
            </div>
          </div>
        )}
        {activeTab === 'mips' && (
          <div className="code-panel">
            <div className="panel-tools panel-tools--right">
              <button className="button button--compact" type="button" onClick={onCopyMips} disabled={!outputs.mips.trim()}>
                Copy MIPS
              </button>
            </div>
            <div className="code-panel__editor">
              <ReadonlyCodePane value={outputs.mips} language="mips" emptyText="No MIPS output yet" />
            </div>
          </div>
        )}
        {activeTab === 'console' && <ConsoleView stdout={outputs.stdout} stderr={outputs.stderr} />}
        {activeTab === 'raw' && (
          <div className="raw-panel">
            <div className="raw-tabs">
              {rawFiles.map((file) => (
                <button
                  key={file.key}
                  className={`raw-tab ${rawFile === file.key ? 'raw-tab--active' : ''}`}
                  type="button"
                  onClick={() => onSelectRawFile(file.key)}
                >
                  {file.label}
                </button>
              ))}
            </div>
            <pre className="raw-pre">{rawValue(rawFile, outputs) || 'No output yet'}</pre>
          </div>
        )}
      </div>
    </section>
  );
}

function ConsoleView({ stdout, stderr }: { stdout: string; stderr: string }) {
  if (!stdout.trim() && !stderr.trim()) {
    return <div className="empty-state">No console output yet</div>;
  }

  return (
    <div className="console-view">
      <section>
        <h3>stdout</h3>
        <pre className="raw-pre">{stdout || '(empty)'}</pre>
      </section>
      <section>
        <h3>stderr</h3>
        <pre className="raw-pre raw-pre--stderr">{stderr || '(empty)'}</pre>
      </section>
    </div>
  );
}

function rawValue(file: SourceFileKey, outputs: CompilerOutputs) {
  if (file === 'token') {
    return outputs.token;
  }
  if (file === 'tree') {
    return outputs.tree;
  }
  if (file === 'midcode') {
    return outputs.midcode;
  }
  if (file === 'mips') {
    return outputs.mips;
  }
  if (file === 'error') {
    return outputs.error;
  }
  return '';
}

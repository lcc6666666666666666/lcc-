import type { CompileStatus } from '../types';

interface CompileToolbarProps {
  status: CompileStatus;
  onCompile: () => void;
  onResetExample: () => void;
  onClearOutput: () => void;
}

const statusLabels: Record<CompileStatus, string> = {
  idle: 'Idle',
  compiling: 'Compiling',
  success: 'Success',
  error: 'Error',
};

export default function CompileToolbar({
  status,
  onCompile,
  onResetExample,
  onClearOutput,
}: CompileToolbarProps) {
  return (
    <header className="toolbar">
      <div className="toolbar__title">SNL Compiler Web IDE</div>
      <div className="toolbar__actions">
        <button className="button button--primary" type="button" onClick={onCompile} disabled={status === 'compiling'}>
          {status === 'compiling' ? 'Compiling...' : 'Compile'}
        </button>
        <button className="button" type="button" onClick={onResetExample} disabled={status === 'compiling'}>
          Reset Example
        </button>
        <button className="button" type="button" onClick={onClearOutput} disabled={status === 'compiling'}>
          Clear Output
        </button>
        <span className="shortcut">Ctrl/Cmd + Enter</span>
        <span className={`compile-status compile-status--${status}`}>{statusLabels[status]}</span>
      </div>
    </header>
  );
}

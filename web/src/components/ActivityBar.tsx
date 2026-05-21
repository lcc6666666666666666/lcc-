import type { SourceFileKey } from '../types';

interface ActivityBarProps {
  activeFile: SourceFileKey;
  onSelectFile: (file: SourceFileKey) => void;
}

const files: Array<{ key: SourceFileKey; label: string; kind: string }> = [
  { key: 'source', label: 'source.snl', kind: 'SNL' },
  { key: 'token', label: 'token.txt', kind: 'TOK' },
  { key: 'tree', label: 'tree.txt', kind: 'AST' },
  { key: 'midcode', label: 'midcode.txt', kind: 'IR' },
  { key: 'mips', label: 'mips.txt', kind: 'ASM' },
  { key: 'error', label: 'error.txt', kind: 'ERR' },
];

export default function ActivityBar({ activeFile, onSelectFile }: ActivityBarProps) {
  return (
    <aside className="activity-bar">
      <div className="activity-bar__section-title">Explorer</div>
      <nav className="file-list" aria-label="Output files">
        {files.map((file) => (
          <button
            key={file.key}
            className={`file-list__item ${activeFile === file.key ? 'file-list__item--active' : ''}`}
            type="button"
            onClick={() => onSelectFile(file.key)}
          >
            <span className={`file-badge file-badge--${file.key}`}>{file.kind}</span>
            <span>{file.label}</span>
          </button>
        ))}
      </nav>
    </aside>
  );
}

import type { Diagnostic } from '../types';

interface ProblemsPanelProps {
  diagnostics: Diagnostic[];
  onSelectLine: (line: number) => void;
}

export default function ProblemsPanel({ diagnostics, onSelectLine }: ProblemsPanelProps) {
  if (diagnostics.length === 0) {
    return <div className="empty-state">No problems detected</div>;
  }

  return (
    <div className="table-wrap">
      <table className="data-table">
        <thead>
          <tr>
            <th>Severity</th>
            <th>Stage</th>
            <th>Line</th>
            <th>Message</th>
          </tr>
        </thead>
        <tbody>
          {diagnostics.map((diagnostic, index) => (
            <tr key={`${diagnostic.line}-${diagnostic.message}-${index}`} onClick={() => onSelectLine(diagnostic.line)}>
              <td>
                <span className={`severity severity--${diagnostic.severity}`}>{diagnostic.severity}</span>
              </td>
              <td>{diagnostic.stage}</td>
              <td>{diagnostic.line}</td>
              <td>{diagnostic.message}</td>
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
}

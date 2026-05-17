import { useMemo, useState } from 'react';
import type { TokenRow } from '../types';

interface TokenTableProps {
  rows: TokenRow[];
  onSelectLine: (line: number) => void;
}

export function parseTokens(tokenText: string): TokenRow[] {
  return tokenText
    .split(/\r?\n/)
    .map((line, index) => {
      const match = line.match(/^\s*(\d+)\s+(\S+)\s*(.*)$/);
      if (!match) {
        return null;
      }

      return {
        index,
        line: Number(match[1]),
        type: match[2],
        lexeme: match[3] ?? '',
      };
    })
    .filter((row): row is TokenRow => row !== null);
}

export default function TokenTable({ rows, onSelectLine }: TokenTableProps) {
  const [query, setQuery] = useState('');
  const normalizedQuery = query.trim().toLowerCase();
  const filteredRows = useMemo(() => {
    if (!normalizedQuery) {
      return rows;
    }
    return rows.filter(
      (row) =>
        row.type.toLowerCase().includes(normalizedQuery) ||
        row.lexeme.toLowerCase().includes(normalizedQuery) ||
        String(row.line).includes(normalizedQuery),
    );
  }, [normalizedQuery, rows]);

  return (
    <div className="tokens-panel">
      <div className="panel-tools">
        <input
          className="filter-input"
          value={query}
          onChange={(event) => setQuery(event.target.value)}
          placeholder="Filter type or lexeme"
          type="search"
        />
        <span className="panel-count">{filteredRows.length} / {rows.length}</span>
      </div>
      {rows.length === 0 ? (
        <div className="empty-state">No tokens yet</div>
      ) : (
        <div className="table-wrap">
          <table className="data-table">
            <thead>
              <tr>
                <th>Line</th>
                <th>Type</th>
                <th>Lexeme</th>
              </tr>
            </thead>
            <tbody>
              {filteredRows.map((row) => (
                <tr key={`${row.index}-${row.type}-${row.lexeme}`} onClick={() => onSelectLine(row.line)}>
                  <td>{row.line}</td>
                  <td>
                    <span className={`token-badge token-badge--${tokenClass(row.type)}`}>{row.type}</span>
                  </td>
                  <td className="lexeme-cell">{row.lexeme}</td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )}
    </div>
  );
}

function tokenClass(type: string) {
  const normalized = type.toUpperCase();
  if (normalized === 'ID') {
    return 'id';
  }
  if (normalized === 'INTC') {
    return 'int';
  }
  if (normalized === 'CHARC') {
    return 'char';
  }
  if (normalized.includes('ERROR')) {
    return 'error';
  }
  if (
    [
      'PROGRAM',
      'TYPE',
      'VAR',
      'PROCEDURE',
      'BEGIN',
      'END',
      'ARRAY',
      'OF',
      'RECORD',
      'IF',
      'THEN',
      'ELSE',
      'FI',
      'WHILE',
      'DO',
      'ENDWH',
      'READ',
      'WRITE',
      'RETURN',
      'INTEGER',
      'CHAR',
    ].includes(normalized)
  ) {
    return 'keyword';
  }
  return 'symbol';
}
